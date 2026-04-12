#ifndef NATIVE_FILE_SYSTEM_H
#define NATIVE_FILE_SYSTEM_H

#include "Common/AsciiString.h"
#include <string>
#include <filesystem>
#include <cstdio>

class NativeFileSystem {
public:
    static FILE* fopen(const AsciiString& path, const char* mode);
    static FILE* fopen(const std::string& path, const char* mode);
    static FILE* fopen(const char* path, const char* mode);

    static bool exists(const std::string& path);
    static bool exists(const AsciiString& path);

    static std::string get_safe_path(const std::string& path);
    static std::string get_engine_path(const std::string& path);
    static std::string normalize_path(const std::string& path);
    static void remove_all_in_directory(const std::string& path);

    static bool create_directory(const std::string& path);
    static bool create_directory(const AsciiString& path);

    static bool create_directories(const std::string& path);
    static bool create_directories(const AsciiString& path);

    static bool is_directory(const std::string& path);
    static bool is_regular_file(const std::string& path);

    static void remove(const std::string& path);
    static void remove_all(const std::string& path);
    
    static void copy(const std::string& from, const std::string& to, std::filesystem::copy_options options);

    static std::uintmax_t file_size(const std::string& path);
    static bool get_file_info(const std::string& path, uint32_t& sizeHigh, uint32_t& sizeLow, uint32_t& timeHigh, uint32_t& timeLow);
    static bool get_file_info(const AsciiString& path, uint32_t& sizeHigh, uint32_t& sizeLow, uint32_t& timeHigh, uint32_t& timeLow);
    
    static void list_files(const std::string& directory, const std::string& searchExt, bool recursive, std::vector<std::string>& outFiles);
    static void list_directories(const std::string& directory, std::vector<std::string>& outDirs);
};

#endif // NATIVE_FILE_SYSTEM_H
