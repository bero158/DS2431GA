#include "fs_esp_idf.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <esp_spiffs.h>
#include "settings.h"

// ESP32's newlib declares fnmatch() in <fnmatch.h> but doesn't link it, so
// implement the small subset ('*' and '?') that Dir::ls patterns need.
static bool matchesPattern(const char *pattern, const char *name) {
    if (*pattern == '\0') return *name == '\0';
    if (*pattern == '*') {
        while (*pattern == '*') pattern++;
        if (*pattern == '\0') return true;
        for (; *name != '\0'; name++) {
            if (matchesPattern(pattern, name)) return true;
        }
        return false;
    }
    if (*name == '\0') return false;
    if (*pattern == '?' || *pattern == *name) return matchesPattern(pattern + 1, name + 1);
    return false;
}

File::File(const String &path, const char *mode) { f_ = fopen(path.c_str(), mode); }
File::~File() { close(); }
File::File(File &&o) noexcept : f_(o.f_) { o.f_ = nullptr; }

File::operator bool() const { return f_ != nullptr; }

size_t File::write(const void *d, size_t n) { return fwrite(d, 1, n, f_); }
size_t File::write(const String &s)         { return write(s.c_str(), s.length()); }
size_t File::read(void *d, size_t n)        { return fread(d, 1, n, f_); }

String File::readLine() {
    char buf[256];
    return fgets(buf, sizeof(buf), f_) ? String(buf) : String();
}

void File::close() {
    if (f_) {
        fclose(f_);
        f_ = nullptr;
    }
}

bool Filesystem::mount(boolean format_if_mount_failed) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_FOLDER,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = format_if_mount_failed
    };
    esp_err_t err = esp_vfs_spiffs_register(&conf);
    return err == ESP_OK;
}


bool Filesystem::unmount() {
    return esp_vfs_spiffs_unregister(NULL) == ESP_OK;
}

bool Filesystem::format() {
    return esp_spiffs_format(NULL) == ESP_OK;
}

bool Dir::md(const String &path) {
    return mkdir(path.c_str(), 0755) == 0;
}

bool Dir::rd(const String &path) {
    return rmdir(path.c_str()) == 0;
}

bool Dir::rm(const String &file) {
    return remove(file.c_str()) == 0;
}

std::vector<String> Dir::ls(const String &path) {
    std::vector<String> out;
    size_t lastSlash = path.lastIndexOf('/');
    String dir = (lastSlash != -1) ? path.substring(0, lastSlash + 1) : String("./");
    String pattern = (lastSlash != -1) ? path.substring(lastSlash + 1) : path;
    
    if (DIR *d = opendir(dir.c_str())) {
        while (dirent *e = readdir(d)) {
            if (matchesPattern(pattern.c_str(), e->d_name)) {
                out.push_back(e->d_name);
            }
        }
        closedir(d);
    }
    return out;
}
