# Compilador y opciones de compilación
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Archivos fuente del proyecto (incluye todas las subcarpetas)
SRC = main.cpp \
      memoria/particionamiento.cpp \
      memoria/paginacion.cpp \
      procesos/procesos.cpp \
      hilos/hilos.cpp

# Transforma la lista de .cpp en objetos .o
OBJ = $(SRC:.cpp=.o)

# Nombre del ejecutable final
TARGET = miniso

# Regla principal por defecto
all: $(TARGET)

# Enlace de objetos para crear el ejecutable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Compilación de cada archivo individual .cpp a .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regla para limpiar archivos generados
clean:
	rm -f $(OBJ) $(TARGET)
