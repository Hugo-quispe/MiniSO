# MiniSO

**MiniSO** es una aplicación de consola desarrollada en **C++** para **Linux** que permite ejecutar y observar conceptos fundamentales de los Sistemas Operativos mediante tres módulos integrados en un único ejecutable con menú principal:

| Módulo | Qué hace | Naturaleza |
|---|---|---|
| **Gestión de procesos** | Crea procesos hijos con `fork()`, ejecuta programas con `execl()` y los espera con `waitpid()` | Real (llamadas al sistema de Linux) |
| **Hilos y concurrencia** | Problema productor-consumidor con búfer acotado, `mutex` y variables de condición | Real (hilos de Linux) |
| **Gestión de memoria** | Particionamiento contiguo (First-Fit) y paginación con tabla de páginas | Simulación (estructuras de datos) |

Proyecto grupal de la asignatura **Sistemas Operativos**.

**Integrantes:** NINA ESTUCO, William Andre 	2024-119006 · PINO ROJAS, Raul Farid Banchello	2024-119009 · ORDOÑO QUISPE, Hugo Antonio	2024-119049 ·

---

## Requisitos

- **Sistema operativo:** Linux (probado en Ubuntu).
- **Compilador:** `g++` con soporte para C++11 o superior.
- **Herramienta de construcción:** GNU Make.
- No se usan bibliotecas externas: solo la biblioteca estándar de C++ y las interfaces POSIX (`unistd.h`, `sys/wait.h`).

Instalación de las herramientas en Ubuntu/Debian:

```bash
sudo apt update
sudo apt install build-essential
```

## Compilación y ejecución

```bash
git clone <URL-DEL-REPOSITORIO>
cd MiniSO
make            # genera el ejecutable ./miniso
./miniso        # inicia la aplicación
make clean      # elimina objetos y ejecutable
```

> Si el enlazado falla por símbolos de hilos en una versión antigua de glibc, agregue `-pthread` a `CXXFLAGS` en el `Makefile`.

## Estructura del proyecto

```
MiniSO/
├── main.cpp                    # Menú principal e integración de módulos
├── Makefile                    # Compilación de todos los módulos
├── README.md
├── procesos/
│   ├── procesos.h
│   └── procesos.cpp            # Módulo I: fork, exec, waitpid
├── hilos/
│   ├── hilos.h
│   └── hilos.cpp               # Módulo II: productor-consumidor
├── memoria/
│   ├── memoria.h
│   ├── particionamiento.cpp    # Módulo III-A: First-Fit
│   └── paginacion.cpp          # Módulo III-B: paginación
└── docs/
    └── pruebas/                # Capturas de las pruebas
```

## Uso

Al iniciar se muestra el menú principal:

```
1. Gestion de procesos
2. Gestion de hilos y concurrencia
3. Gestion de memoria: Particionamiento (First-Fit)
4. Gestion de memoria: Paginacion (Marcos y Tablas)
0. Salir
```

### 1. Gestión de procesos

No solicita datos. Crea dos procesos hijos de forma concurrente:

- **Hijo 1:** ejecuta `/bin/ls -l`.
- **Hijo 2:** ejecuta `/bin/date`.

Cada hijo muestra su **PID** y **PPID**. El padre espera a ambos con `waitpid()` y muestra el código de salida de cada uno. Los errores de `fork()`, `execl()` y `waitpid()` se informan por consola.

Para contrastar con Linux, puede abrir otra terminal y usar `ps` o `pstree -p`.

### 2. Gestión de hilos y concurrencia

Siempre se ejecutan **2 productores y 2 consumidores** (hilos reales) que comparten un búfer FIFO de capacidad limitada, protegido con `std::mutex` y coordinado con dos `std::condition_variable`.

| Opción | Descripción |
|---|---|
| 1 | Simulación personalizada: elementos (1–10000), capacidad del búfer (1–1000) y perfil de velocidad |
| 2 | Prueba A: capacidad 3, 20 elementos |
| 3 | Prueba B: capacidad 5, 30 elementos (consumidores lentos, el búfer tiende a llenarse) |
| 4 | Prueba C: capacidad 2, 10 elementos (productores lentos, el búfer tiende a vaciarse) |
| 5 | Ejecuta las pruebas A, B y C |
| 0 | Volver al menú principal |

**Perfiles de velocidad:** equilibrada, consumidores lentos, productores lentos y sin pausas.

Durante la ejecución se muestran los mensajes de generación, inserción, retiro, espera por búfer lleno o vacío y finalización de cada hilo. Al terminar se imprime un **resumen**: ocupación máxima, cantidad de esperas, elementos producidos, consumidos, pendientes, perdidos y duplicados, y la validación final (`CORRECTO` / `INCORRECTO`).

### 3. Particionamiento de memoria (First-Fit)

La primera vez se solicita la **memoria total**. El estado se conserva mientras el programa esté abierto.

| Opción | Descripción |
|---|---|
| 1 | Mostrar estado de la memoria (bloques libres y ocupados) |
| 2 | Solicitar memoria: ID de proceso y tamaño |
| 3 | Liberar la memoria de un proceso (une bloques libres adyacentes) |
| 4 | Reconfigurar el tamaño total (reinicia la simulación) |
| 0 | Volver |

- Un proceso ocupa **un solo bloque contiguo**.
- Si una solicitud no se puede atender pero la memoria libre total alcanza, se informa **fragmentación externa**.

### 4. Paginación

Valores iniciales: memoria de 1024 bytes y páginas de 64 bytes (16 marcos).

| Opción | Descripción |
|---|---|
| 1 | Mostrar el estado de los marcos |
| 2 | Asignar memoria a un proceso (ID y tamaño en bytes) |
| 3 | Ver la tabla de páginas de un proceso |
| 4 | Traducir una dirección lógica a física |
| 5 | Liberar la memoria de un proceso |
| 6 | Configurar tamaño de memoria y de página (reinicia la simulación) |
| 0 | Volver |

Traducción: `página = DL / T`, `desplazamiento = DL % T`, `DF = marco × T + desplazamiento`. Las direcciones fuera del espacio del proceso se informan como error.

#### Ejemplo de verificación manual

Memoria 256 bytes, página 64 bytes. P1 a P4 de 64 bytes; se liberan P1 y P3; P5 de 128 bytes recibe página 0 → marco 0 y página 1 → marco 2.

| Dirección lógica | Cálculo manual | Dirección física |
|---|---|---|
| 10 | 0×64 + 10 | 10 |
| 70 | 2×64 + 6 | 134 |
| 127 | 2×64 + 63 | 191 |

## Pruebas y evidencias

Las capturas de ejecución en Linux están en [`docs/pruebas/`](docs/pruebas/):

| Captura | Qué muestra |
|---|---|
| `Compilacion.png` | Compilación con `make` |
| `PruebaProcesos.png` | Procesos hijos, PID/PPID y finalización |
| `PruebaSincronizacionHilos.png` | Productores y consumidores sincronizados |
| `PruebaParticionamientoAsignacion.png` | Asignación First-Fit |
| `PruebaParticionamientoLiberacion.png` | Liberación y reutilización de bloques |
| `PruebaPaginacionAsignacion.png`, `PruebaPaginacionAsignacion2.png` | Asignación de páginas y marcos |
| `PruebaConstruccionTablas.png` | Reconfiguración de la paginación |
| `PruebaTraduccionDirecciones.png` | Traducción lógica → física |

## Limitaciones conocidas

- Los programas del módulo de procesos (`ls`, `date`) son fijos.
- El número de productores y consumidores es fijo (2 y 2).
- Los módulos de memoria no validan por completo la entrada: un tamaño de página igual a 0 provoca un error de división entre cero.
- En paginación, todas las páginas se asignan al crear el proceso; no hay paginación por demanda ni reemplazo de páginas.
- Terminar la entrada (Ctrl+D) dentro del menú de hilos puede dejar el menú principal repitiéndose.

## Herramientas

C++ (`g++ -Wall -std=c++11`), GNU Make, POSIX (`fork`, `exec`, `waitpid`), `std::thread` / `std::mutex` / `std::condition_variable`, Git y GitHub.
