#include <vector>
#include <cerrno>       
#include <iostream>     
#include <unistd.h>     
#include <fcntl.h>     
#include <sys/stat.h>  
#include <cstdint>      

#include "copy_logic.h" 

// 1.4.1 Implementación
bool check_args_count(int argc) {
    if (argc < 3) { 
      std::cerr << "Uso: " << "copy origen1 [origen2 ...] DESTINO\n";
        return false;
    }
    return true;
}

// 1.4.2. Implementación
bool is_directory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

// 1.4.3. Implementación
std::string get_filename(const std::string& path) {
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos) {
        return path;
    }
    return path.substr(last_slash + 1);
}

// 1.4.4. Implementación
std::expected<void, std::system_error> copy_file(
    const std::string& src_path, 
    const std::string& dest_path, 
    mode_t dst_perms) {

    // --- 1. Abrir archivo ORIGEN (Lectura) ---
    int fd_src = open(src_path.c_str(), O_RDONLY);
    if (fd_src == -1) {
        return std::unexpected(std::system_error(
            errno, std::system_category(), "error al abrir el archivo de origen: " + src_path));
    }

    // --- 2. Abrir archivo DESTINO (Escritura) ---
    int flags = O_CREAT | O_WRONLY | O_TRUNC;
    mode_t perms = dst_perms & 0777; // Asegurarnos de que solo son bits de permiso

    int fd_dest = open(dest_path.c_str(), flags, perms);
    if (fd_dest == -1) {
        close(fd_src);
        return std::unexpected(std::system_error(
            errno, std::system_category(), "error al abrir el archivo de destino: " + dest_path));
    }

    // --- 3. Bucle de Lectura/Escritura ---
    const size_t BUFFER_SIZE = 65536; // 64 KiB
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    ssize_t bytes_read;

    while (true) {
        // --- 3a. Leer del origen ---
        bytes_read = read(fd_src, buffer.data(), BUFFER_SIZE);

        if (bytes_read == -1) { // Error
            close(fd_src);
            close(fd_dest);
            return std::unexpected(std::system_error(
                errno, std::system_category(), "error al leer el archivo de origen"));
        }

        if (bytes_read == 0) { // Fin del archivo (EOF)
            break; 
        }

        // --- 3b. Escribir en el destino ---
        ssize_t total_written = 0;
        while (total_written < bytes_read) {
            ssize_t bytes_written = write(fd_dest, 
                                        buffer.data() + total_written, 
                                        bytes_read - total_written);

            if (bytes_written == -1) {
                close(fd_src);
                close(fd_dest);
                return std::unexpected(std::system_error(
                    errno, std::system_category(), "error al escribir en el archivo de destino"));
            }
            total_written += bytes_written;
        }
    }

    // --- 4. Cerrar descriptores ---
    if (close(fd_src) == -1 || close(fd_dest) == -1) {
        return std::unexpected(std::system_error(
            errno, std::system_category(), "error al cerrar los archivos"));
    }

    // --- 5. Éxito ---
    return {};
}