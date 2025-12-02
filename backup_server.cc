#include "copy_logic.h"

#include <iostream>
#include <atomic>
#include <csignal>      
#include <cstring>   
#include <unistd.h>   
#include <fcntl.h>     
#include <sys/stat.h>   
#include <climits>      

std::atomic<bool> quit_requested{false};

// FUNCIONES AUXILIARES

void term_signal_handler(int signum) {
    (void)signum;
    const char msg[] = "backup-server: señal de terminación recibida, cerrando...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    quit_requested = true;
}

std::expected<pid_t, std::system_error> read_server_pid(const std::string& pid_file_path) {
    int fd = open(pid_file_path.c_str(), O_RDONLY);
    if (fd == -1) return std::unexpected(std::system_error(errno, std::system_category()));
    char buffer[64];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytes_read <= 0) return std::unexpected(std::system_error(errno, std::system_category()));
    buffer[bytes_read] = '\0';
    return std::stoi(buffer);
}

bool is_server_running(pid_t pid) {
    if (kill(pid, 0) == 0) return true;
    return errno != ESRCH;
}

std::expected<void, std::system_error> create_fifo(const std::string& fifo_path) {
    unlink(fifo_path.c_str());
    if (mkfifo(fifo_path.c_str(), 0666) == -1) {
        return std::unexpected(std::system_error(errno, std::system_category()));
    }
    return {};
}

std::expected<void, std::system_error> write_pid_file(const std::string& pid_file_path) {
    int fd = open(pid_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) return std::unexpected(std::system_error(errno, std::system_category()));
    std::string pid_str = std::to_string(getpid()) + "\n";
    write(fd, pid_str.c_str(), pid_str.length());
    close(fd);
    return {};
}

std::expected<void, std::system_error> setup_signal_handler() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
        return std::unexpected(std::system_error(errno, std::system_category()));
    }

    struct sigaction sa;
    sa.sa_handler = term_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1 ||
        sigaction(SIGTERM, &sa, nullptr) == -1 ||
        sigaction(SIGHUP, &sa, nullptr) == -1 ||
        sigaction(SIGQUIT, &sa, nullptr) == -1) {
        return std::unexpected(std::system_error(errno, std::system_category()));
    }
    return {};
}

std::expected<std::string, std::system_error> read_path_from_fifo(int fifo_fd) {
    std::string path;
    char c;
    int count = 0;
    while (count < PATH_MAX) {
        ssize_t res = read(fifo_fd, &c, 1);
        if (res == -1) {
            if (errno == EINTR) continue;
            return std::unexpected(std::system_error(errno, std::system_category()));
        }
        if (res == 0) break;
        if (c == '\n') break;
        path += c;
        count++;
    }
    return path;
}

// BUCLE PRINCIPAL 
// (modif)

void run_server(int fifo_fd, const std::string& backup_dir) {
    sigset_t wait_set;
    sigemptyset(&wait_set);
    sigaddset(&wait_set, SIGUSR1);

    siginfo_t info; // Aquí se guardarán los datos del cliente (PID)

    while (!quit_requested) {
        // Esperamos señal del cliente
        int signum = sigwaitinfo(&wait_set, &info);

        if (signum == -1) {
            if (errno == EINTR) continue;
            std::cerr << "Error en sigwaitinfo\n";
            break;
        }

        // Obtenemos el PID del cliente para responderle
        pid_t client_pid = info.si_pid;

        // Leemos ruta
        auto path_result = read_path_from_fifo(fifo_fd);
        if (!path_result.has_value()) {
            std::cerr << "Error leyendo FIFO. Notificando fallo al cliente...\n";
            kill(client_pid, SIGUSR2); // Error
            continue;
        }

        std::string src_path = path_result.value();
        if (src_path.empty()) {
            kill(client_pid, SIGUSR2); // Error (ruta vacía)
            continue;
        }

        // Construimos destino
        std::string filename = get_filename(src_path);
        std::string dest_path = backup_dir + "/" + filename;

        // Copiamos y notificamos
        auto copy_result = copy_file(src_path, dest_path, 0666);
        
        if (!copy_result.has_value()) {
            std::cerr << "Fallo al copiar " << src_path << ": " << copy_result.error().what() << "\n";
            kill(client_pid, SIGUSR2); // Notificar FALLO (SIGUSR2)
        } else {
            std::cout << "backup-server: backup completado para PID " << client_pid << "\n";
            kill(client_pid, SIGUSR1); // Notificar ÉXITO (SIGUSR1)
        }
    }
}

// MAIN DEL SERVIDOR

int main(int argc, char* argv[]) {
    std::string backup_dir_path = (argc > 1) ? argv[1] : ".";
    std::string work_dir = get_work_dir_path();
    
    if (work_dir.empty()) {
        std::cerr << "backup-server: error: BACKUP_WORK_DIR no definida\n";
        return EXIT_FAILURE;
    }
    if (!is_directory(backup_dir_path)) {
        std::cerr << "backup-server: error: directorio destino no existe\n";
        return EXIT_FAILURE;
    }

    // Check PID existente
    std::string pid_path = get_pid_file_path();
    auto pid_result = read_server_pid(pid_path);
    if (pid_result.has_value() && is_server_running(pid_result.value())) {
        std::cerr << "backup-server: error: ya hay un servidor ejecutándose\n";
        return EXIT_FAILURE;
    }

    // Crear FIFO
    std::string fifo_path = get_fifo_path();
    if (!create_fifo(fifo_path).has_value()) return EXIT_FAILURE;

    // Escribir PID
    if (!write_pid_file(pid_path).has_value()) return EXIT_FAILURE;

    // Señales
    if (!setup_signal_handler().has_value()) return EXIT_FAILURE;

    // Abrir FIFO (O_RDWR para evitar bloqueo por EOF)
    int fifo_fd = open(fifo_path.c_str(), O_RDWR);
    if (fifo_fd == -1) return EXIT_FAILURE;

    std::cout << "backup-server: esperando solicitudes de backup en " << backup_dir_path << "\n";

    run_server(fifo_fd, backup_dir_path);

    // Limpieza
    close(fifo_fd);
    unlink(fifo_path.c_str());
    unlink(pid_path.c_str());

    return EXIT_SUCCESS;
}