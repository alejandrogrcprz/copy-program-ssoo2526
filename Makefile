# Define el compilador y las banderas
CXX = g++
# -std=c++23 es necesario para std::expected
# -Wall -Wextra son avisos de "buena práctica"
# -g añade información de depuración
CXXFLAGS = -std=c++23 -Wall -Wextra -g

# El nombre del programa final
TARGET = copy

# Los ficheros ".o" (objeto) que necesitamos
OBJECTS = main.o copy_logic.o

# La regla principal: para crear el TARGET, depende de los OBJECTS
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Regla para crear main.o: depende de main.cc y copy_logic.h
main.o: main.cc copy_logic.h
	$(CXX) $(CXXFLAGS) -c main.cc -o main.o

# Regla para crear copy_logic.o: depende de copy_logic.cc y copy_logic.h
copy_logic.o: copy_logic.cc copy_logic.h
	$(CXX) $(CXXFLAGS) -c copy_logic.cc -o copy_logic.o

# Regla "clean" para borrar los ficheros generados
clean:
	rm -f $(TARGET) $(OBJECTS)