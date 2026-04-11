#include "Common/System/NativeFileSystem.h"
#include <algorithm>

namespace {
    std::string GetSafePath(const std::string& path) {
        std::string safePath = path;
#ifdef __APPLE__
        std::replace(safePath.begin(), safePath.end(), '\\', '/');
#endif
        return safePath;
    }
}

FILE* NativeFileSystem::fopen(const AsciiString& path, const char* mode) {
    if (path.isEmpty()) return nullptr;
    return ::fopen(GetSafePath(path.str()).c_str(), mode);
}

FILE* NativeFileSystem::fopen(const std::string& path, const char* mode) {
    if (path.empty()) return nullptr;
    return ::fopen(GetSafePath(path).c_str(), mode);
}

FILE* NativeFileSystem::fopen(const char* path, const char* mode) {
    if(!path) return nullptr;
    return ::fopen(GetSafePath(path).c_str(), mode);
}

bool NativeFileSystem::exists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::exists(GetSafePath(path), ec);
}

bool NativeFileSystem::exists(const AsciiString& path) {
    if (path.isEmpty()) return false;
    std::error_code ec;
    return std::filesystem::exists(GetSafePath(path.str()), ec);
}

std::string NativeFileSystem::get_safe_path(const std::string& path) {
    return GetSafePath(path);
}

void NativeFileSystem::remove_all_in_directory(const std::string& path) {
    std::error_code ec;
    std::string safePath = GetSafePath(path);
    if (std::filesystem::exists(safePath, ec) && std::filesystem::is_directory(safePath, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(safePath)) {
            std::filesystem::remove_all(entry.path(), ec);
        }
    }
}

bool NativeFileSystem::create_directory(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directory(GetSafePath(path), ec);
}

bool NativeFileSystem::create_directory(const AsciiString& path) {
    if (path.isEmpty()) return false;
    std::error_code ec;
    return std::filesystem::create_directory(GetSafePath(path.str()), ec);
}

bool NativeFileSystem::is_directory(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_directory(GetSafePath(path), ec);
}

bool NativeFileSystem::is_regular_file(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(GetSafePath(path), ec);
}

void NativeFileSystem::remove(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(GetSafePath(path), ec);
}

void NativeFileSystem::remove_all(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove_all(GetSafePath(path), ec);
}

void NativeFileSystem::copy(const std::string& from, const std::string& to, std::filesystem::copy_options options) {
    std::error_code ec;
    std::filesystem::copy(GetSafePath(from), GetSafePath(to), options, ec);
}

std::uintmax_t NativeFileSystem::file_size(const std::string& path) {
    std::error_code ec;
    return std::filesystem::file_size(GetSafePath(path), ec);
}
