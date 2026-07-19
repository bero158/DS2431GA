#include "fs_esp_idf.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <esp_spiffs.h>

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
        .base_path = "/spiffs",
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
    if (DIR *d = opendir(path.c_str())) {
        while (dirent *e = readdir(d)) out.push_back(e->d_name);
        closedir(d);
    }
    return out;
}
