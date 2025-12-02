CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -g

# Definimos los 3 programas que queremos crear
TARGETS = copy backup-server backup

# Objetos comunes (la lógica compartida)
COMMON_OBJS = copy_logic.o

# Regla 'all': compila todo lo de la lista TARGETS
all: $(TARGETS)

# --- PROGRAMA 1: COPY (Actividad 1) ---
# Usa main.cc + copy_logic
copy: main.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o copy main.o $(COMMON_OBJS)

# --- PROGRAMA 2: SERVIDOR (Actividad 2) ---
# Usa backup_server.cc + copy_logic
backup-server: backup_server.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o backup-server backup_server.o $(COMMON_OBJS)

# --- PROGRAMA 3: CLIENTE (Actividad 2) ---
# Usa backup_client.cc + copy_logic
backup: backup_client.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o backup backup_client.o $(COMMON_OBJS)

# --- REGLAS DE COMPILACIÓN DE OBJETOS ---
main.o: main.cc copy_logic.h
	$(CXX) $(CXXFLAGS) -c main.cc

backup_server.o: backup_server.cc copy_logic.h
	$(CXX) $(CXXFLAGS) -c backup_server.cc

backup_client.o: backup_client.cc copy_logic.h
	$(CXX) $(CXXFLAGS) -c backup_client.cc

copy_logic.o: copy_logic.cc copy_logic.h
	$(CXX) $(CXXFLAGS) -c copy_logic.cc

# --- LIMPIEZA ---
clean:
	rm -f $(TARGETS) *.o *.exe
	rm -rf prueba backup *.dat *.txt *.fifo *.pid