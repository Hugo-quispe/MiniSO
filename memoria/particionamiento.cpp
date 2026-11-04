#include <iostream>
#include <vector>
#include "memoria.h"

using namespace std;

// Estructura para representar un bloque de memoria
struct Bloque {
    int inicio;
    int tamano;
    bool libre;
    int idProceso; // -1 si está libre, de lo contrario el ID del proceso que lo ocupa
};

// Variables globales del módulo
static vector<Bloque> listaBloques;
static int tamanoMemoriaTotal = 1024; // Valor por defecto configurable

// Función para mostrar el estado actual de los bloques libres y ocupados
void mostrarEstadoMemoria() {
    cout << "\n================ ESTADO DE LA MEMORIA ================" << endl;
    cout << "Memoria Total: " << tamanoMemoriaTotal << " unidades" << endl;
    cout << "------------------------------------------------------" << endl;
    cout << "ID Bloque | Inicio | Fin  | Tamaño | Estado    | Proceso" << endl;
    cout << "------------------------------------------------------" << endl;
    for (size_t i = 0; i < listaBloques.size(); i++) {
        cout << "   [" << i << "]    | " 
             << listaBloques[i].inicio << "    | " 
             << (listaBloques[i].inicio + listaBloques[i].tamano) << "   | " 
             << listaBloques[i].tamano << "    | " 
             << (listaBloques[i].libre ? "LIBRE   " : "OCUPADO ") << " | ";
        if (listaBloques[i].libre) {
            cout << "-" << endl;
        } else {
            cout << "P" << listaBloques[i].idProceso << endl;
        }
    }
    cout << "------------------------------------------------------" << endl;
}

// Algoritmo de asignación contigua: First-Fit con restricción de bloque único por proceso
void solicitarMemoriaFirstFit(int idProceso, int tamanoSolicitado) {
    // 1. Validar que el proceso no tenga ya memoria asignada previamente
    for (const auto& b : listaBloques) {
        if (!b.libre && b.idProceso == idProceso) {
            cout << "[ERROR] El Proceso " << idProceso << " ya tiene un bloque asignado en memoria. "
                 << "En particionamiento contiguo, un proceso ocupa un único bloque continuo." << endl;
            return;
        }
    }

    // 2. Buscar el primer bloque libre que cumpla con el tamaño (First-Fit)
    for (size_t i = 0; i < listaBloques.size(); i++) {
        if (listaBloques[i].libre && listaBloques[i].tamano >= tamanoSolicitado) {
            if (listaBloques[i].tamano == tamanoSolicitado) {
                listaBloques[i].libre = false;
                listaBloques[i].idProceso = idProceso;
            } else {
                // Dividir el bloque si sobra espacio
                Bloque nuevoBloque;
                nuevoBloque.inicio = listaBloques[i].inicio + tamanoSolicitado;
                nuevoBloque.tamano = listaBloques[i].tamano - tamanoSolicitado;
                nuevoBloque.libre = true;
                nuevoBloque.idProceso = -1;

                listaBloques[i].tamano = tamanoSolicitado;
                listaBloques[i].libre = false;
                listaBloques[i].idProceso = idProceso;

                listaBloques.insert(listaBloques.begin() + i + 1, nuevoBloque);
            }
            cout << "[ÉXITO] Proceso " << idProceso << " asignado correctamente en un bloque continuo de " << tamanoSolicitado << " unidades." << endl;
            return;
        }
    }

    // 3. Identificación de Fragmentación Externa si falla la asignación
    int memoriaLibreTotal = 0;
    for (const auto& b : listaBloques) {
        if (b.libre) memoriaLibreTotal += b.tamano;
    }

    cout << "[ERROR] No se pudo asignar memoria al Proceso " << idProceso << "." << endl;
    if (memoriaLibreTotal >= tamanoSolicitado) {
        cout << "-> ¡Fragmentación Externa Detectada! Hay suficiente memoria libre total (" 
             << memoriaLibreTotal << "), pero ningún bloque contiguo es lo suficientemente grande." << endl;
    } else {
        cout << "-> Memoria insuficiente en el sistema." << endl;
    }
}

// Liberar memoria del proceso y unir bloques contiguos libres (Coalescence)
void liberarMemoria(int idProceso) {
    bool encontrado = false;
    for (size_t i = 0; i < listaBloques.size(); i++) {
        if (!listaBloques[i].libre && listaBloques[i].idProceso == idProceso) {
            listaBloques[i].libre = true;
            listaBloques[i].idProceso = -1;
            encontrado = true;
            cout << "[LIBERADO] El bloque del Proceso " << idProceso << " fue liberado exitosamente." << endl;
        }
    }

    if (!encontrado) {
        cout << "[AVISO] No se encontró ningún proceso activo con el ID " << idProceso << "." << endl;
        return;
    }

    // Unión de bloques libres contiguos adyacentes
    vector<Bloque> listaCompacta;
    for (size_t i = 0; i < listaBloques.size(); i++) {
        if (listaCompacta.empty()) {
            listaCompacta.push_back(listaBloques[i]);
        } else {
            if (listaCompacta.back().libre && listaBloques[i].libre) {
                listaCompacta.back().tamano += listaBloques[i].tamano;
            } else {
                listaCompacta.push_back(listaBloques[i]);
            }
        }
    }
    listaBloques = listaCompacta;
}

// Función principal del módulo conectada con el menú
void ejecutarParticionamiento() {
    int opcion;

    if (listaBloques.empty()) {
        cout << "\nIngrese la cantidad configurable de memoria total: ";
        cin >> tamanoMemoriaTotal;
        listaBloques.push_back({0, tamanoMemoriaTotal, true, -1});
    }

    do {
        cout << "\n=== SUBMENÚ: PARTICIONAMIENTO DE MEMORIA (FIRST-FIT) ===" << endl;
        cout << "1. Mostrar estado de la memoria" << endl;
        cout << "2. Solicitar memoria (Nuevo Proceso)" << endl;
        cout << "3. Liberar memoria (Finalizar Proceso)" << endl;
        cout << "4. Reconfigurar tamaño total de memoria" << endl;
        cout << "0. Volver al menú principal" << endl;
        cout << "Seleccione una opción: ";
        cin >> opcion;

        switch (opcion) {
            case 1:
                mostrarEstadoMemoria();
                break;
            case 2: {
                int id, tam;
                cout << "Ingrese el ID del proceso: ";
                cin >> id;
                cout << "Ingrese el tamaño de memoria requerido: ";
                cin >> tam;
                if (tam > 0) {
                    solicitarMemoriaFirstFit(id, tam);
                } else {
                    cout << "El tamaño debe ser mayor al 0." << endl;
                }
                break;
            }
            case 3: {
                int id;
                cout << "Ingrese el ID del proceso a liberar: ";
                cin >> id;
                liberarMemoria(id);
                break;
            }
            case 4: {
                cout << "Ingrese el nuevo tamaño total de la memoria: ";
                cin >> tamanoMemoriaTotal;
                listaBloques.clear();
                listaBloques.push_back({0, tamanoMemoriaTotal, true, -1});
                cout << "Memoria reinicializada con un tamaño de " << tamanoMemoriaTotal << endl;
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
