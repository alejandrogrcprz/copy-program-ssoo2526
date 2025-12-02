#pragma once

#include <string>
#include <expected>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>

// FUNCIONES DE LA UNIDAD 1 
bool is_directory(const std::string& path);
std::string get_filename(const std::string& path);

[[nodiscard]]
std::expected<void, std::system_error> copy_file(
    const std::string& src_path, 
    const std::string& dest_path, 
    mode_t dst_perms);

// FUNCIONES DE LA UNIDAD 2

// Obtiene la variable de entorno BACKUP_WORK_DIR
std::string get_work_dir_path();

// Retorna la ruta completa de la FIFO (dir_trabajo + "/backup.fifo")
std::string get_fifo_path();

// Retorna la ruta completa del archivo PID (dir_trabajo + "/backup-server.pid")
std::string get_pid_file_path();

// Convierte ruta relativa a absoluta usando realpath()
std::expected<std::string, std::system_error> get_absolute_path(const std::string& path);

// Verifica si un archivo existe
bool file_exists(const std::string& path);

// Verifica si es un archivo regular (wrapper de S_ISREG)
bool is_regular_file(const std::string& path);