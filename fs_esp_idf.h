#pragma once

#include <cstddef>
#include <cstdio>
#include <vector>
#include <Arduino.h>

// RAII wrapper around a C FILE* stream.
class File {
    FILE *f_ = nullptr;
public:
    File(const String &path, const char *mode);
    ~File();
    File(const File &) = delete;
    File &operator=(const File &) = delete;
    File(File &&o) noexcept;

    explicit operator bool() const;

    size_t write(const void *d, size_t n);
    size_t write(const String &s);
    size_t read(void *d, size_t n);

    String readLine();
    void close();
};

// Directory create/remove/list helper.
class Dir {
public:
    static bool md(const String &path);
    static bool rd(const String &path);
    static bool rm(const String &file);
    static std::vector<String> ls(const String &path);
};

// Mount/unmount helper for the ESP-IDF SPIFFS filesystem.
class Filesystem {
public:
    bool mount(boolean format_if_mount_failed = true);
    bool unmount();
    bool format();
};
