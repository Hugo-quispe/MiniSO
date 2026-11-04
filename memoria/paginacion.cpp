#include <iostream>
#include <vector>
#include <iomanip>
#include "memoria.h"

using namespace std;

// Estructura para representar un marco en la memoria física
struct MarcoFisico {
    int idMarco;
    bool libre;
    int idProceso;     // -1 si está libre, de lo contrario el PID que lo ocupa
    int numPaginaVirtual; // Qué página lógica del proceso aloja
};

// Estructura para una entrada en la tabla de páginas
struct EntradaTablaPagina {
    int paginaVirtual;
    int marcoFisico;
    bool valida;
};

// Estructura que agrupa la tabla de páginas de un proceso específico
struct ProcesoPaginado {
    int idProceso;
    int tamanoBytes;
    int numPaginas;
    vector<EntradaTablaPagina> tablaPaginas;
};

// Variables globales del módulo de paginación
static int tamanoMemoriaFisica = 1024; // Tamaño total por defecto
static int tamanoPagina = 64;          // Tamaño de cada página / marco por defecto
static int totalMarcos = 16;           // Calculado como tamanoMemoriaFisica / tamanoPagina

static vector<MarcoFisico> memoriaFisica;
static vector<ProcesoPaginado> listaProcesosPaginados;

// Inicializar la memoria física si está vacía
static void inicializarPaginacionSiEsNecesario() {
    if (memoriaFisica.empty()) {
        totalMarcos = tamanoMemoriaFisica / tamanoPagina;
        memoriaFisica.resize(totalMarcos);
        for (int i = 0; i < totalMarcos; i++) {
            memoriaFisica[i] = {i, true, -1, -1};
        }
    }
}

// 1. Mostrar estado de la memoria física (Marcos)
void mostrarMemoriaFisica() {
    inicializarPaginacionSiEsNecesario();
    cout << "\n================ ESTADO DE LA MEMORIA FÍSICA (PAGINACIÓN) ================" << endl;
    cout << "Memoria Total: " << tamanoMemoriaFisica << " bytes | Tamaño de Página/Marco: " << tamanoPagina << " bytes" << endl;
    cout << "Total de Marcos: " << totalMarcos << endl;
    cout << "--------------------------------------------------------------------------" << endl;
    cout << "Marco ID | Estado    | Proceso Asignado | Página Virtual" << endl;
    cout << "--------------------------------------------------------------------------" << endl;
    for (int i = 0; i < totalMarcos; i++) {
        cout << "   [" << setw(2) << memoriaFisica[i].idMarco << "]   | ";
        if (memoriaFisica[i].libre) {
            cout << "LIBRE     | -                | -" << endl;
        } else {
            cout << "OCUPADO   | P" << setw(15) << left << memoriaFisica[i].idProceso << " | Pág " << memoriaFisica[i].numPaginaVirtual << right << endl;
        }
    }
    cout << "--------------------------------------------------------------------------" << endl;
}

// Buscar un proceso en la lista de paginados por su ID
int buscarProcesoPaginado(int idProceso) {
    for (size_t i = 0; i < listaProcesosPaginados.size(); i++) {
        if (listaProcesosPaginados[i].idProceso == idProceso) {
            return i;
        }
    }
    return -1;
}

// 2. Asignar memoria a un nuevo proceso dividiéndolo en páginas
void asignarMemoriaPaginacion(int idProceso, int tamanoBytes) {
    inicializarPaginacionSiEsNecesario();

    if (buscarProcesoPaginado(idProceso) != -1) {
        cout << "[ERROR] El Proceso " << idProceso << " ya tiene memoria paginada asignada." << endl;
        return;
    }

    // Calcular cuántas páginas necesita el proceso (redondeando hacia arriba)
    int numPaginas = (tamanoBytes + tamanoPagina - 1) / tamanoPagina;

    // Contar marcos libres disponibles en la memoria física
    int marcosLibresDisponibles = 0;
    for (const auto& marco : memoriaFisica) {
        if (marco.libre) marcosLibresDisponibles++;
    }

    if (numPaginas > marcosLibresDisponibles) {
        cout << "[ERROR] Memoria física insuficiente. El proceso requiere " << numPaginas 
             << " marcos, pero solo hay " << marcosLibresDisponibles << " marcos libres." << endl;
        return;
    }

    // Crear la estructura del proceso paginado
    ProcesoPaginado nuevoProceso;
    nuevoProceso.idProceso = idProceso;
    nuevoProceso.tamanoBytes = tamanoBytes;
    nuevoProceso.numPaginas = numPaginas;

    // Asignar marcos físicos disponibles (de forma no contigua)
    int paginasAsignadas = 0;
    for (int i = 0; i < totalMarcos && paginasAsignadas < numPaginas; i++) {
        if (memoriaFisica[i].libre) {
            // Ocupar el marco físico
            memoriaFisica[i].libre = false;
            memoriaFisica[i].idProceso = idProceso;
            memoriaFisica[i].numPaginaVirtual = paginasAsignadas;

            // Registrar en la tabla de páginas del proceso
            EntradaTablaPagina entrada;
            entrada.paginaVirtual = paginasAsignadas;
            entrada.marcoFisico = i;
            entrada.valida = true;
            nuevoProceso.tablaPaginas.push_back(entrada);

            paginasAsignadas++;
        }
    }

    listaProcesosPaginados.push_back(nuevoProceso);
    cout << "[ÉXITO] Proceso " << idProceso << " asignado con " << numPaginas 
         << " página(s) (" << tamanoBytes << " bytes) mediante asignación paginada." << endl;
}

// 3. Mostrar la Tabla de Páginas de un Proceso específico
void mostrarTablaPaginas(int idProceso) {
    int idx = buscarProcesoPaginado(idProceso);
    if (idx == -1) {
        cout << "[AVISO] No se encontró el proceso P" << idProceso << " en la memoria paginada." << endl;
        return;
    }

    const ProcesoPaginado& proc = listaProcesosPaginados[idx];
    cout << "\n--- TABLA DE PÁGINAS: PROCESO P" << proc.idProceso << " ---" << endl;
    cout << "Tamaño Total: " << proc.tamanoBytes << " bytes | Total Páginas: " << proc.numPaginas << endl;
    cout << "----------------------------------------" << endl;
    cout << "Página Virtual | Marco Físico | Estado" << endl;
    cout << "----------------------------------------" << endl;
    for (const auto& entry : proc.tablaPaginas) {
        cout << "      Pág " << setw(2) << entry.paginaVirtual 
             << "    |    Marco " << setw(2) << entry.marcoFisico 
             << "  | " << (entry.valida ? "Válida" : "Inválida") << endl;
    }
    cout << "----------------------------------------" << endl;
}

// 4. Traducir Dirección Lógica (Virtual) a Dirección Física
void traducirDireccion(int idProceso, int direccionLogica) {
    int idx = buscarProcesoPaginado(idProceso);
    if (idx == -1) {
        cout << "[ERROR] El Proceso " << idProceso << " no existe en el sistema de paginación." << endl;
        return;
    }

    const ProcesoPaginado& proc = listaProcesosPaginados[idx];
    int maxMemoriaProceso = proc.numPaginas * tamanoPagina;

    if (direccionLogica < 0 || direccionLogica >= maxMemoriaProceso) {
        cout << "[ERROR] Violación de segmento: La dirección lógica " << direccionLogica 
             << " está fuera del espacio de direcciones del proceso (Máx: " << maxMemoriaProceso << " bytes)." << endl;
        return;
    }

    // Cálculo de página virtual y desplazamiento (offset)
    int paginaVirtual = direccionLogica / tamanoPagina;
    int desplazamiento = direccionLogica % tamanoPagina;

    // Buscar el marco físico correspondiente en la tabla de páginas
    int marcoFisico = -1;
    for (const auto& entry : proc.tablaPaginas) {
        if (entry.paginaVirtual == paginaVirtual && entry.valida) {
            marcoFisico = entry.marcoFisico;
            break;
        }
    }

    if (marcoFisico == -1) {
        cout << "[ERROR] Fallo de página (Page Fault): La página virtual " << paginaVirtual << " no es válida." << endl;
        return;
    }

    // Cálculo de la dirección física final
    int direccionFisica = (marcoFisico * tamanoPagina) + desplazamiento;

    cout << "\n---------------- TRADUCCIÓN DE DIRECCIÓN ----------------" << endl;
    cout << " Proceso ID          : P" << idProceso << endl;
    cout << " Dirección Lógica    : " << direccionLogica << endl;
    cout << " --------------------------------------------------------" << endl;
    cout << " -> Página Virtual   : " << paginaVirtual << endl;
    cout << " -> Desplazamiento   : " << desplazamiento << endl;
    cout << " -> Marco Físico     : " << marcoFisico << endl;
    cout << " --------------------------------------------------------" << endl;
    cout << " Dirección Física    : " << direccionFisica << " (Marco " << marcoFisico << " + Offset " << desplazamiento << ")" << endl;
    cout << "--------------------------------------------------------" << endl;
}

// 5. Liberar memoria de un proceso (Liberar marcos físicos y eliminar tabla)
void liberarMemoriaPaginacion(int idProceso) {
    int idx = buscarProcesoPaginado(idProceso);
    if (idx == -1) {
        cout << "[AVISO] No se encontró ningún proceso activo con el ID " << idProceso << " en paginación." << endl;
        return;
    }

    // Liberar los marcos físicos asociados en la memoria
    for (int i = 0; i < totalMarcos; i++) {
        if (memoriaFisica[i].idProceso == idProceso) {
            memoriaFisica[i].libre = true;
            memoriaFisica[i].idProceso = -1;
            memoriaFisica[i].numPaginaVirtual = -1;
        }
    }

    // Eliminar el proceso de la lista de paginados
    listaProcesosPaginados.erase(listaProcesosPaginados.begin() + idx);
    cout << "[LIBERADO] Memoria y tabla de páginas del Proceso " << idProceso << " liberadas exitosamente." << endl;
}

// Función principal del submenú conectada al main
void ejecutarPaginacion() {
    int opcion;
    inicializarPaginacionSiEsNecesario();

    do {
        cout << "\n=== SUBMENÚ: GESTIÓN DE MEMORIA (PAGINACIÓN) ===" << endl;
        cout << "1. Mostrar estado de la memoria física y marcos" << endl;
        cout << "2. Asignar memoria a un proceso (Nueva Paginación)" << endl;
        cout << "3. Ver tabla de páginas de un proceso" << endl;
        cout << "4. Traducir dirección lógica a física" << endl;
        cout << "5. Liberar memoria de un proceso" << endl;
        cout << "6. Configurar tamaño de memoria y página" << endl;
        cout << "0. Volver al menú principal" << endl;
        cout << "Seleccione una opción: ";
        cin >> opcion;

        switch (opcion) {
            case 1:
                mostrarMemoriaFisica();
                break;
            case 2: {
                int id, tam;
                cout << "Ingrese el ID del proceso: ";
                cin >> id;
                cout << "Ingrese el tamaño requerido en bytes: ";
                cin >> tam;
                if (tam > 0) {
                    asignarMemoriaPaginacion(id, tam);
                } else {
                    cout << "El tamaño debe ser mayor a 0." << endl;
                }
                break;
            }
            case 3: {
                int id;
                cout << "Ingrese el ID del proceso: ";
                cin >> id;
                mostrarTablaPaginas(id);
                break;
            }
            case 4: {
                int id, dirLogica;
                cout << "Ingrese el ID del proceso: ";
                cin >> id;
                cout << "Ingrese la dirección lógica (virtual) a traducir: ";
                cin >> dirLogica;
                traducirDireccion(id, dirLogica);
                break;
            }
            case 5: {
                int id;
                cout << "Ingrese el ID del proceso a liberar: ";
                cin >> id;
                liberarMemoriaPaginacion(id);
                break;
            }
            case 6: {
                cout << "Ingrese el nuevo tamaño total de la memoria física (bytes): ";
                cin >> tamanoMemoriaFisica;
                cout << "Ingrese el tamaño de cada página / marco (bytes): ";
                cin >> tamanoPagina;
                memoriaFisica.clear();
                listaProcesosPaginados.clear();
                inicializarPaginacionSiEsNecesario();
                cout << "Sistema de paginación reconfigurado exitosamente." << endl;
                break;
            }
            case 0:
                cout << "Regresando al menú principal..." << endl;
                break;
            default:
                cout << "Opción no válida. Intente nuevamente." << endl;
        }
    } while (opcion != 0);
}
