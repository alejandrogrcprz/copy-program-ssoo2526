#include "copy_logic.h"

#include <iostream>
#include <csignal>      
#include <cstring>      
#include <unistd.h>    
#include <fcntl.h>     

// FUNCIONES AUXILIARES

bool check_args(int argc) {
    if (argc != 2) {
        std::cerr << "Uso: backup ARCHIVO\n";
        return false;
    }
    return true;
}

std::expected<int, std::system_error> open_fifo_write(const std::string& fifo_path) {
    int fd = open(fifo_path.c_str(), O_WRONLY);
    if (fd == -1) {
        return std::unexpected(std::system_error(errno, std::system_category()));
    }
    return fd;
}

std::expected<void, std::system_error> write_path_to_fifo(int fifo_fd, const std::string& file_path) {
    std::string payload = file_path + "\n";
    ssize_t written = write(fifo_fd, payload.c_str(), payload.length());
    
    if (written == -1) {
        return std::unexpected(std::system_error(errno, std::system_category()));
    }
    return {};
}

// MAIN DEL CLIENTE

int main(int argc, char* argv[]) {
    // 1. Validar Argumentos
    if (!check_args(argc)) return EXIT_FAILURE;

    std::string source_file = argv[1];

    // 2. Validar Entorno
    std::string work_dir = get_work_dir_path();
    if (work_dir.empty()) {
        std::cerr << "backup: error: BACKUP_WORK_DIR no está definida\n";
        return EXIT_FAILURE;
    }

    // 3. Validar Archivo Origen
    if (!file_exists(source_file)) {
        std::cerr << "backup: error: el archivo " << source_file << " no existe\n";
        return EXIT_FAILURE;
    }
    if (!is_regular_file(source_file)) {
        std::cerr << "backup: error: " << source_file << " no es un archivo regular\n";
        return EXIT_FAILURE;
    }

    // 4. Leer PID del Servidor
    std::string pid_path = get_pid_file_path();
    int pid_fd = open(pid_path.c_str(), O_RDONLY);
    if (pid_fd == -1) {
        std::cerr << "backup: error: servidor no iniciado (no existe PID file)\n";
        return EXIT_FAILURE;
    }
    
    char pid_buf[64];
    ssize_t bytes = read(pid_fd, pid_buf, sizeof(pid_buf)-1);
    close(pid_fd);
    
    if (bytes <= 0) {
         std::cerr << "backup: error leyendo PID file\n";
         return EXIT_FAILURE;
    }
    pid_buf[bytes] = '\0';
    pid_t server_pid = std::stoi(pid_buf);

    // 5. Verificar si el servidor corre
    if (kill(server_pid, 0) == -1) {
        std::cerr << "backup: error: el servidor (PID " << server_pid << ") no responde\n";
        return EXIT_FAILURE;
    }

    // 6. BLOQUEAR SEÑALES(modif)
    // Bloqueamos SIGUSR1 y SIGUSR2 antes de interactuar para no perder la respuesta
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGUSR1); // Éxito
    sigaddset(&set, SIGUSR2); // Fallo
    
    if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
        std::cerr << "backup: error bloqueando señales\n";
        return EXIT_FAILURE;
    }

    // 7. Abrir FIFO
    std::string fifo_path = get_fifo_path();
    auto fifo_res = open_fifo_write(fifo_path);
    if (!fifo_res.has_value()) {
        std::cerr << "backup: error abriendo FIFO: " << fifo_res.error().what() << "\n";
        return EXIT_FAILURE;
    }
    int fifo_fd = fifo_res.value();

    // 8. Convertir a Ruta Absoluta
    auto abs_path_res = get_absolute_path(source_file);
    if (!abs_path_res.has_value()) {
        std::cerr << "backup: error obteniendo ruta absoluta\n";
        close(fifo_fd);
        return EXIT_FAILURE;
    }
    std::string abs_path = abs_path_res.value();

    // 9. Escribir en FIFO
    auto write_res = write_path_to_fifo(fifo_fd, abs_path);
    if (!write_res.has_value()) {
        std::cerr << "backup: error escribiendo en FIFO\n";
        close(fifo_fd);
        return EXIT_FAILURE;
    }

    // 10. Enviar solicitud al servidor
    if (kill(server_pid, SIGUSR1) == -1) {
        std::cerr << "backup: error enviando señal al servidor\n";
        close(fifo_fd);
        return EXIT_FAILURE;
    }

    // 11. ESPERAR RESPUESTA (modif)
    std::cout << "backup: solicitud enviada, esperando confirmación...\n";

    siginfo_t info;
    // Nos quedamos dormidos hasta recibir USR1 o USR2
    int sig = sigwaitinfo(&set, &info);
    close(fifo_fd); // Ya no necesitamos la FIFO

    if (sig == -1) {
        std::cerr << "backup: error esperando respuesta\n";
        return EXIT_FAILURE;
    }

    // 12. Interpretar respuesta (modif)
    if (sig == SIGUSR1) {
        std::cout << "backup: ÉXITO. Copia realizada correctamente.\n";
        return EXIT_SUCCESS;
    } else if (sig == SIGUSR2) {
        std::cerr << "backup: FALLO. El servidor no pudo copiar el archivo.\n";
        return EXIT_FAILURE;
    } else {
        std::cerr << "backup: Señal inesperada recibida: " << sig << "\n";
        return EXIT_FAILURE;
    }
}