# 📋 Programa `copy` (Sistemas Operativos)

* **Autor:** Alejandro García Pérez ([alejandrogrcprz](https://github.com/alejandrogrcprz))

---

Este proyecto es la implementación de la **Actividad 1: Manipulación de archivos** de la asignatura de Sistemas Operativos (2025/2026).

El objetivo es desarrollar un programa en C++ llamado `copy` que replica la funcionalidad básica del comando `cp` de POSIX, interactuando directamente con el sistema operativo a bajo nivel.

## 🎯 Objetivo de la Práctica

El propósito principal no es solo copiar un archivo, sino aprender a manipular archivos utilizando **exclusivamente las llamadas al sistema** del sistema operativo (API POSIX).

El programa debe ser robusto y capaz de manejar archivos de cualquier tamaño.

## ✨ Características de Implementación

Según las especificaciones del guion, el programa cumple con los siguientes requisitos:

* **Funcionalidad Básica:**
    * `copy ORIGEN DESTINO`.
    * Si `DESTINO` existe y es un archivo, se sobrescribe.
    * Si `DESTINO` existe y es un directorio, `ORIGEN` se copia dentro de él.
    * Si `DESTINO` no existe, se crea como un archivo nuevo.

* **Requisitos Técnicos (El Núcleo del Ejercicio):**
    * **Uso Exclusivo de API POSIX:** El programa utiliza *únicamente* llamadas al sistema de bajo nivel como `open()`, `read()`, `write()`, `stat()` y `close()`.
    * **Funciones Prohibidas:** Está explícitamente prohibido usar funciones de biblioteca de alto nivel como `std::ifstream`, `fopen()`, `fread()`, `fwrite()`, etc..
    * **Eficiencia:** La copia se realiza en bloques de 64 KiB (65536 bytes) para poder manejar archivos muy grandes que no cabrían en memoria.
    * **Manejo de Errores Moderno:** La propagación de errores desde las funciones de bajo nivel se gestiona utilizando `std::expected` y `std::system_error`, como se describe en el Apéndice B del guion.

## 🚀 Uso y Casos de Prueba

La ejecución sigue el formato estándar `ORIGEN DESTINO`. El programa también maneja varios casos de error especificados:

```bash
# Caso 1: Copia estándar
./copy archivo_origen.txt archivo_destino.txt

# Caso 2: Copia a un directorio
mkdir mi_carpeta
./copy archivo_origen.txt mi_carpeta/

# --- Casos de Error Esperados ---

# Error: Faltan argumentos
$ ./copy archivo_origen.txt
copy: se deben indicar los archivos ORIGEN DESTINO

# Error: Copia a sí mismo
$ ./copy testfile.dat testfile.dat
copy: el archivo ORIGEN y DESTINO no pueden ser el mismo

# Error: Origen no existe
$ ./copy noexiste.dat destino.dat
copy: error al abrir el archivo de origen: No such file or directory

# Error: Permisos de destino
$ ./copy testfile.dat /
copy: error al abrir el archivo de destino: Permission denied
```

Este proyecto se basa en el guion de la práctica proporcionado por Jesús Torres para la asignatura de Sistemas Operativos.
