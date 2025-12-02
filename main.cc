#include <iostream>
#include <string>
#include <cstdlib>      
#include <sys/stat.h>   
#include <cstring>      
#include <cerrno>       

#include "copy_logic.h"

bool check_args_count(int argc) {
    if (argc < 3) {
        std::cerr << "copy: se deben indicar los archivos ORIGEN... DESTINO\n";
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    // 1. Comprobar argumentos
    if (!check_args_count(argc)) {
        return EXIT_FAILURE;
    }

    std::string destino_root = argv[argc - 1];
    bool destino_es_dir = is_directory(destino_root);

    if (argc > 3 && !destino_es_dir) {
        std::cerr << "copy: al copiar multiples archivos el destino debe ser un directorio\n";
        return EXIT_FAILURE;
    }

    // 2. Iterar sobre cada archivo de origen
    for (int i = 1; i < argc - 1; ++i) {
        std::string origen = argv[i];
        std::string destino = destino_root;

        // 3. Obtener atributos del ORIGEN
        struct stat stat_origen;
        if (stat(origen.c_str(), &stat_origen) == -1) {
            std::cerr << "copy: error al leer el archivo de origen: " << std::strerror(errno) << '\n';
            return EXIT_FAILURE;
        }

        // 4. Comprobar si DESTINO es un directorio
       if (destino_es_dir) {
         destino = destino + "/" + get_filename(origen);
       }

        // 5. Comprobar si son el mismo archivo
        struct stat stat_destino;
        if (stat(destino.c_str(), &stat_destino) == 0) {
            if (stat_origen.st_dev == stat_destino.st_dev && 
                stat_origen.st_ino == stat_destino.st_ino) {
                std::cerr << "copy: el archivo ORIGEN y DESTINO no pueden ser el mismo\n";
                return EXIT_FAILURE;
            }
        } else {
            if (errno != ENOENT) {
                std::cerr << "copy: error al leer el archivo de destino: " << std::strerror(errno) << '\n';
                return EXIT_FAILURE;
            }
        }

        // 6. Copiar el archivo
        auto result = copy_file(origen, destino, stat_origen.st_mode);

        // 7. Manejar el resultado de la copia
        if (!result.has_value()) {
            const auto& error = result.error();
            std::cerr << "copy: " << error.what() << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}