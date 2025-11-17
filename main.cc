#include <iostream>
#include <string>
#include <cstdlib>      // Para EXIT_SUCCESS y EXIT_FAILURE
#include <sys/stat.h>   // Para stat()
#include <cstring>      // Para std::strerror
#include <cerrno>       // Para errno

// Incluimos nuestra propia cabecera
#include "copy_logic.h"

// --- Función Principal ---
int main(int argc, char* argv[]) {
    // 1. Comprobar SOLO el número de argumentos
    if (!check_args_count(argc)) {
        return EXIT_FAILURE;
    }

    std::string origen = argv[1];
    std::string destino = argv[2];

    // 2. Obtener atributos del ORIGEN
    struct stat stat_origen;
    if (stat(origen.c_str(), &stat_origen) == -1) {
        // El origen no existe o no se puede leer
        std::cerr << "copy: error al leer el archivo de origen: " << std::strerror(errno) << '\n';
        return EXIT_FAILURE;
    }

    // 3. Comprobar si DESTINO es un directorio
    if (is_directory(destino)) {
        // Construir la nueva ruta de destino
        destino = destino + "/" + get_filename(origen);
    }
    
// 4. Comprobar si son el mismo archivo (DESPUÉS de construir la ruta final)
    struct stat stat_destino;
    
    if (stat(destino.c_str(), &stat_destino) == 0) {
        // El archivo de destino SÍ existe. Comprobemos si es el mismo.
        if (stat_origen.st_dev == stat_destino.st_dev && 
            stat_origen.st_ino == stat_destino.st_ino) {
            std::cerr << "copy: el archivo ORIGEN y DESTINO no pueden ser el mismo\n";
            return EXIT_FAILURE;
        }
    } else {
        // stat() falló. ¿Por qué?
        // Si falló porque el archivo no existe (ENOENT), está bien.
        // Si falló por cualquier otra razón (ej. Permission denied al leer
        // los atributos del archivo 'solo_lectura.dat'), ¡es un error!
        if (errno != ENOENT) {
            std::cerr << "copy: error al leer el archivo de destino: " << std::strerror(errno) << '\n';
            return EXIT_FAILURE;
        }
        // Si era ENOENT, no hacemos nada. Dejamos que copy_file cree el archivo.
    }

    // 5. Copiar el archivo (pasando los permisos del original)
    auto result = copy_file(origen, destino, stat_origen.st_mode);

    // 6. Manejar el resultado de la copia
    if (!result.has_value()) {
        const auto& error = result.error();
        std::cerr << "copy: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}