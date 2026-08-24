#include "GameNetwork/GeneralsOnline/DiscordRichPresence_Apple.h"

#ifdef __APPLE__

#include "GameNetwork/GeneralsOnline/NGMP_include.h"
#include "GameNetwork/GeneralsOnline/json.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

constexpr auto RECONNECT_INTERVAL = std::chrono::seconds(5);
constexpr int SOCKET_CANDIDATE_COUNT = 10;
constexpr size_t MAX_FRAME_BYTES = 64 * 1024;
constexpr int WRITE_POLL_TIMEOUT_MS = 50;
constexpr size_t FRAME_HEADER_BYTES = sizeof(uint32_t) * 2;

enum class Opcode : uint32_t {
  Handshake = 0,
  Frame = 1,
  Close = 2,
  Ping = 3,
  Pong = 4,
};

std::string GetRuntimeDirectory() {
  for (const char *variable : {"XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP"}) {
    const char *value = getenv(variable);
    if (value == nullptr || value[0] == '\0') {
      continue;
    }

    std::string directory(value);
    while (!directory.empty() && directory.back() == '/') {
      directory.pop_back();
    }
    return directory;
  }

  return "/tmp";
}

int ConnectToSocket(const std::string &path) {
  sockaddr_un address = {};
  if (path.size() >= sizeof(address.sun_path)) {
    return -1;
  }

  const int descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
  if (descriptor < 0) {
    return -1;
  }

  const int noSignalOnBrokenPipe = 1;
  setsockopt(descriptor, SOL_SOCKET, SO_NOSIGPIPE, &noSignalOnBrokenPipe,
             sizeof(noSignalOnBrokenPipe));

  address.sun_family = AF_UNIX;
  memcpy(address.sun_path, path.c_str(), path.size());

  if (connect(descriptor, reinterpret_cast<const sockaddr *>(&address),
              sizeof(address)) < 0) {
    close(descriptor);
    return -1;
  }

  fcntl(descriptor, F_SETFL, O_NONBLOCK);
  return descriptor;
}

void AssignText(nlohmann::json &target, const char *key, const char *value) {
  if (value == nullptr || value[0] == '\0') {
    return;
  }

  target[key] = value;
}

nlohmann::json BuildTimestamps(const DiscordRichPresence &presence) {
  nlohmann::json timestamps = nlohmann::json::object();
  if (presence.startTimestamp > 0) {
    timestamps["start"] = presence.startTimestamp;
  }

  if (presence.endTimestamp > 0) {
    timestamps["end"] = presence.endTimestamp;
  }

  return timestamps;
}

nlohmann::json BuildAssets(const DiscordRichPresence &presence) {
  nlohmann::json assets = nlohmann::json::object();
  AssignText(assets, "large_image", presence.largeImageKey);
  AssignText(assets, "large_text", presence.largeImageText);
  AssignText(assets, "small_image", presence.smallImageKey);
  AssignText(assets, "small_text", presence.smallImageText);
  return assets;
}

nlohmann::json BuildParty(const DiscordRichPresence &presence) {
  nlohmann::json party = nlohmann::json::object();
  if (presence.partyId == nullptr || presence.partyId[0] == '\0') {
    return party;
  }

  party["id"] = presence.partyId;
  if (presence.partyMax > 0) {
    party["size"] = {(std::max)(presence.partySize, 1), presence.partyMax};
  }

  return party;
}

nlohmann::json BuildSecrets(const DiscordRichPresence &presence) {
  nlohmann::json secrets = nlohmann::json::object();
  AssignText(secrets, "match", presence.matchSecret);
  AssignText(secrets, "join", presence.joinSecret);
  AssignText(secrets, "spectate", presence.spectateSecret);
  return secrets;
}

void AssignSection(nlohmann::json &activity, const char *key,
                   const nlohmann::json &section) {
  if (section.empty()) {
    return;
  }

  activity[key] = section;
}

nlohmann::json BuildActivity(const DiscordRichPresence &presence) {
  nlohmann::json activity = nlohmann::json::object();
  AssignText(activity, "state", presence.state);
  AssignText(activity, "details", presence.details);
  AssignSection(activity, "timestamps", BuildTimestamps(presence));
  AssignSection(activity, "assets", BuildAssets(presence));
  AssignSection(activity, "party", BuildParty(presence));
  AssignSection(activity, "secrets", BuildSecrets(presence));
  activity["instance"] = presence.instance != 0;
  return activity;
}

class DiscordIpcConnection {
public:
  void Open(const char *applicationId);
  void Close();
  void Poll();
  void SetActivity(const nlohmann::json &activity);
  void ClearActivity();

private:
  enum class State {
    Disconnected,
    Handshaking,
    Connected,
  };

  bool TryConnect();
  void Disconnect();
  bool WriteFrame(Opcode opcode, const std::string &payload);
  bool ReadAvailableBytes();
  void ProcessFrames();
  void HandleFrame(Opcode opcode, const std::string &payload);
  void SendPendingCommand();

  std::string m_applicationId;
  std::string m_readBuffer;
  std::string m_pendingCommand;
  int m_descriptor = -1;
  State m_state = State::Disconnected;
  std::chrono::steady_clock::time_point m_nextConnectAttempt;
  uint64_t m_nonce = 0;
};

void DiscordIpcConnection::Open(const char *applicationId) {
  Close();

  if (applicationId == nullptr || applicationId[0] == '\0') {
    return;
  }

  m_applicationId = applicationId;
  m_nextConnectAttempt = std::chrono::steady_clock::now();
  Poll();
}

void DiscordIpcConnection::Close() {
  Disconnect();
  m_applicationId.clear();
  m_pendingCommand.clear();
}

void DiscordIpcConnection::Disconnect() {
  if (m_descriptor >= 0) {
    close(m_descriptor);
    m_descriptor = -1;
  }

  m_state = State::Disconnected;
  m_readBuffer.clear();
  m_nextConnectAttempt = std::chrono::steady_clock::now() + RECONNECT_INTERVAL;
}

void DiscordIpcConnection::Poll() {
  if (m_applicationId.empty()) {
    return;
  }

  if (m_state == State::Disconnected) {
    if (std::chrono::steady_clock::now() < m_nextConnectAttempt) {
      return;
    }

    if (!TryConnect()) {
      m_nextConnectAttempt =
          std::chrono::steady_clock::now() + RECONNECT_INTERVAL;
      return;
    }
  }

  if (!ReadAvailableBytes()) {
    Disconnect();
    return;
  }

  ProcessFrames();
}

bool DiscordIpcConnection::TryConnect() {
  const std::string directory = GetRuntimeDirectory();
  for (int index = 0; index < SOCKET_CANDIDATE_COUNT; ++index) {
    const int descriptor =
        ConnectToSocket(std::format("{}/discord-ipc-{}", directory, index));
    if (descriptor < 0) {
      continue;
    }

    m_descriptor = descriptor;
    m_state = State::Handshaking;
    m_readBuffer.clear();

    const nlohmann::json handshake = {{"v", 1},
                                      {"client_id", m_applicationId}};
    if (!WriteFrame(Opcode::Handshake, handshake.dump())) {
      Disconnect();
      return false;
    }

    NetworkLog(ELogVerbosity::LOG_RELEASE,
               "[DiscordRPC] Connected to discord-ipc-%d", index);
    return true;
  }

  return false;
}

bool DiscordIpcConnection::WriteFrame(Opcode opcode,
                                      const std::string &payload) {
  if (m_descriptor < 0) {
    return false;
  }

  const uint32_t header[2] = {static_cast<uint32_t>(opcode),
                              static_cast<uint32_t>(payload.size())};
  std::string frame(reinterpret_cast<const char *>(header),
                    FRAME_HEADER_BYTES);
  frame.append(payload);

  size_t written = 0;
  while (written < frame.size()) {
    const ssize_t result =
        send(m_descriptor, frame.data() + written, frame.size() - written, 0);
    if (result > 0) {
      written += static_cast<size_t>(result);
      continue;
    }

    if (result < 0 && errno == EINTR) {
      continue;
    }

    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      pollfd waiter = {m_descriptor, POLLOUT, 0};
      if (poll(&waiter, 1, WRITE_POLL_TIMEOUT_MS) > 0) {
        continue;
      }
    }

    return false;
  }

  return true;
}

bool DiscordIpcConnection::ReadAvailableBytes() {
  char chunk[4096];
  while (true) {
    const ssize_t result = recv(m_descriptor, chunk, sizeof(chunk), 0);
    if (result > 0) {
      m_readBuffer.append(chunk, static_cast<size_t>(result));
      continue;
    }

    if (result == 0) {
      return false;
    }

    if (errno == EINTR) {
      continue;
    }

    return errno == EAGAIN || errno == EWOULDBLOCK;
  }
}

void DiscordIpcConnection::ProcessFrames() {
  while (m_readBuffer.size() >= FRAME_HEADER_BYTES) {
    uint32_t header[2] = {};
    memcpy(header, m_readBuffer.data(), FRAME_HEADER_BYTES);

    const size_t length = header[1];
    if (length > MAX_FRAME_BYTES) {
      Disconnect();
      return;
    }

    if (m_readBuffer.size() < FRAME_HEADER_BYTES + length) {
      return;
    }

    const std::string payload = m_readBuffer.substr(FRAME_HEADER_BYTES, length);
    m_readBuffer.erase(0, FRAME_HEADER_BYTES + length);
    HandleFrame(static_cast<Opcode>(header[0]), payload);

    if (m_descriptor < 0) {
      return;
    }
  }
}

void DiscordIpcConnection::HandleFrame(Opcode opcode,
                                       const std::string &payload) {
  if (opcode == Opcode::Ping) {
    WriteFrame(Opcode::Pong, payload);
    return;
  }

  if (opcode == Opcode::Close) {
    NetworkLog(ELogVerbosity::LOG_RELEASE,
               "[DiscordRPC] Discord closed the connection: %s",
               payload.c_str());
    Disconnect();
    return;
  }

  if (opcode != Opcode::Frame || m_state != State::Handshaking) {
    return;
  }

  const nlohmann::json message =
      nlohmann::json::parse(payload, nullptr, false);
  if (message.is_discarded()) {
    return;
  }

  const auto event = message.find("evt");
  if (event == message.end() || !event->is_string() ||
      event->get<std::string>() != "READY") {
    return;
  }

  m_state = State::Connected;
  NetworkLog(ELogVerbosity::LOG_RELEASE, "[DiscordRPC] Rich Presence ready");
  SendPendingCommand();
}

void DiscordIpcConnection::SendPendingCommand() {
  if (m_state != State::Connected || m_pendingCommand.empty()) {
    return;
  }

  if (!WriteFrame(Opcode::Frame, m_pendingCommand)) {
    Disconnect();
  }
}

void DiscordIpcConnection::SetActivity(const nlohmann::json &activity) {
  nlohmann::json arguments = nlohmann::json::object();
  arguments["pid"] = getpid();
  if (!activity.is_null()) {
    arguments["activity"] = activity;
  }

  nlohmann::json command = nlohmann::json::object();
  command["cmd"] = "SET_ACTIVITY";
  command["nonce"] = std::to_string(++m_nonce);
  command["args"] = arguments;

  m_pendingCommand = command.dump();
  SendPendingCommand();
}

void DiscordIpcConnection::ClearActivity() { SetActivity(nlohmann::json()); }

DiscordIpcConnection &Connection() {
  static DiscordIpcConnection instance;
  return instance;
}

} // namespace

namespace DiscordIpc {

void Initialize(const char *applicationId, DiscordEventHandlers *, int,
                const char *) {
  Connection().Open(applicationId);
}

void Shutdown() { Connection().Close(); }

void RunCallbacks() { Connection().Poll(); }

void UpdatePresence(const DiscordRichPresence *presence) {
  if (presence == nullptr) {
    Connection().ClearActivity();
    return;
  }

  Connection().SetActivity(BuildActivity(*presence));
}

void ClearPresence() { Connection().ClearActivity(); }

} // namespace DiscordIpc

#endif
