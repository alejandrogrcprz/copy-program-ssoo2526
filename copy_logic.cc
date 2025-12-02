#include "copy_logic.h"

#include <vector>
#include <cerrno>
#include <cstdint>
#include <cstdlib>     
#include <fcntl.h>    
#include <unistd.h>     

// IMPLEMENTACIONES UNIDAD 1

bool is_directory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) return false;
    return S_ISDIR(st.st_mode);
}

std::string get_filename(const std::string& path) {
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos) return path;
    return path.substr(last_slash + 1);
}

std::expected<void, std::system_error> copy_file(
    const std::string& src_path, 
    const std::string& dest_path, 
    mode_t dst_perms) {

    // 1. Abrir origen
    int fd_src = open(src_path.c_str(), O_RDONLY);
    if (fd_src == -1) {
        return std::unexpected(std::system_error(errno, std::system_category()));
    }

    // 2. Abrir destino (Crear, Escritura, Truncar si existe)
    int flags = O_CREAT | O_WRONLY | O_TRUNC;
    mode_t perms = dst_perms & 0777; 

    int fd_dest = open(dest_path.c_str(), flags, perms);
    if (fd_dest == -1) {
        close(fd_src);
        return std::unexpected(std::system_error(errno, std::system_category()));
    }

    // 3. Copiar datos
    const size_t BUFFER_SIZE = 65536; 
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    ssize_t bytes_read;

    while (true) {
        bytes_read = read(fd_src, buffer.data(), BUFFER_SIZE);

        if (bytes_read == -1) { // Error lectura
            close(fd_src); close(fd_dest);
            return std::unexpected(std::system_error(errno, std::system_category()));
        }

        if (bytes_read == 0) break; // Fin de archivo

        // Bucle para asegurar escritura completa del bloque
        ssize_t total_written = 0;
        while (total_written < bytes_read) {
            ssize_t bytes_written = write(fd_dest, 
                                        buffer.data() + total_written, 
                                        bytes_read - total_written);

            if (bytes_written == -1) { // Error escritura
                close(fd_src); close(fd_dest);
                return std::unexpected(std::system_error(errno, std::system_category()));
            }
            total_written += bytes_written;
        }
    }

    close(fd_src);
    close(fd_dest);
    return {};
}

// IMPLEMENTACIONES UNIDAD 2

std::string get_work_dir_path() {
    char* value = std::getenv("BACKUP_WORK_DIR");
    return value ? std::string(value) : std::string();
}

std::string get_fifo_path() {
    std::string work_dir = get_work_dir_path();
    return work_dir.empty() ? "" : work_dir + "/backup.fifo";
}

std::string get_pid_file_path() {
    std::string work_dir = get_work_dir_path();
    return work_dir.empty() ? "" : work_dir + "/backup-server.pid";
}

std::expected<std::string, std::system_error> get_absolute_path(const std::string& path) {
    // realpath con nullptr asigna memoria dinámicamente
    char* resolved = realpath(path.c_str(), nullptr);
    
    if (!resolved) {
        return std::unexpected(std::system_error(errno, std::system_category()));
    }

    std::string abs_path(resolved);
    std::free(resolved); // Liberar memoria de realpath
    return abs_path;
}

bool file_exists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

bool is_regular_file(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) return false;
    return S_ISREG(st.st_mode);
}