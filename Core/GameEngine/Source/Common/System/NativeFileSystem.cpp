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
    return NativeFileSystem::fopen(std::string(path.str()), mode);
}

FILE* NativeFileSystem::fopen(const std::string& path, const char* mode) {
    if (path.empty()) return nullptr;
    std::string safePath = GetSafePath(path);
    
    // Automatically create parent directories on macOS if we are opening a file for writing or appending
    if (mode != nullptr && (strchr(mode, 'w') != nullptr || strchr(mode, 'a') != nullptr)) {
        std::error_code ec;
        std::filesystem::path p(safePath);
        std::filesystem::path parent = p.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
            std::filesystem::create_directories(parent, ec);
        }
    }
    
    return ::fopen(safePath.c_str(), mode);
}

FILE* NativeFileSystem::fopen(const char* path, const char* mode) {
    if(!path) return nullptr;
    return NativeFileSystem::fopen(std::string(path), mode);
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

std::string NativeFileSystem::get_engine_path(const std::string& path) {
    std::string safePath = path;
#ifdef __APPLE__
    std::replace(safePath.begin(), safePath.end(), '/', '\\');
#endif
    return safePath;
}

std::string NativeFileSystem::normalize_path(const std::string& path) {
    std::string safePath = GetSafePath(path);
    std::filesystem::path p(safePath);
    return get_engine_path(p.lexically_normal().string());
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

bool NativeFileSystem::create_directories(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directories(GetSafePath(path), ec);
}

bool NativeFileSystem::create_directories(const AsciiString& path) {
    if (path.isEmpty()) return false;
    std::error_code ec;
    return std::filesystem::create_directories(GetSafePath(path.str()), ec);
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

bool NativeFileSystem::get_file_info(const std::string& path, uint32_t& sizeHigh, uint32_t& sizeLow, uint32_t& timeHigh, uint32_t& timeLow) {
    std::string safePath = GetSafePath(path);
    std::error_code ec;
    
    auto f_size = std::filesystem::file_size(safePath, ec);
    if (ec) return false;
    
    auto write_time = std::filesystem::last_write_time(safePath, ec);
    if (ec) return false;
    
    auto time = write_time.time_since_epoch().count();
    timeHigh = time >> 32;
    timeLow = time & UINT32_MAX;
    sizeHigh = f_size >> 32;
    sizeLow = f_size & UINT32_MAX;
    return true;
}

bool NativeFileSystem::get_file_info(const AsciiString& path, uint32_t& sizeHigh, uint32_t& sizeLow, uint32_t& timeHigh, uint32_t& timeLow) {
    if (path.isEmpty()) return false;
    return get_file_info(std::string(path.str()), sizeHigh, sizeLow, timeHigh, timeLow);
}

#ifdef _WIN32
#define STRCASECMP _stricmp
#else
#include <strings.h>
#define STRCASECMP strcasecmp
#endif

void NativeFileSystem::list_files(const std::string& directory, const std::string& searchExt, bool recursive, std::vector<std::string>& outFiles) {
    std::string safeDir = GetSafePath(directory);
    std::error_code ec;
    
    if (!std::filesystem::exists(safeDir, ec) || !std::filesystem::is_directory(safeDir, ec)) {
        return;
    }

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(safeDir, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (entry.is_regular_file(ec)) {
                std::string ext = entry.path().extension().string();
                if (searchExt.empty() || STRCASECMP(ext.c_str(), searchExt.c_str()) == 0) {
                    outFiles.push_back(entry.path().filename().string());
                }
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(safeDir, ec)) {
            if (entry.is_regular_file(ec)) {
                std::string ext = entry.path().extension().string();
                if (searchExt.empty() || STRCASECMP(ext.c_str(), searchExt.c_str()) == 0) {
                    outFiles.push_back(entry.path().filename().string());
                }
            }
        }
    }
}

void NativeFileSystem::list_directories(const std::string& directory, std::vector<std::string>& outDirs) {
    std::string safeDir = GetSafePath(directory);
    std::error_code ec;
    
    if (!std::filesystem::exists(safeDir, ec) || !std::filesystem::is_directory(safeDir, ec)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(safeDir, ec)) {
        if (entry.is_directory(ec)) {
            std::string dirName = entry.path().filename().string();
            if (dirName != "." && dirName != "..") {
                outDirs.push_back(dirName);
            }
        }
    }
}
