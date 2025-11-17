#pragma once // Evita que este fichero se incluya múltiples veces

#include <string>
#include <expected>
#include <system_error>
#include <sys/stat.h>   // Para mode_t

// 1.4.2. Comprobar si DESTINO es un directorio
bool is_directory(const std::string& path);

// 1.4.3. Obtener el nombre del archivo desde una ruta
std::string get_filename(const std::string& path);

// 1.4.4. Copiar el archivo
[[nodiscard]]
std::expected<void, std::system_error> copy_file(
    const std::string& src_path, 
    const std::string& dest_path, 
    mode_t dst_perms);

// 1.4.1. Comprueba solo el número de argumentos
bool check_args_count(int argc);