#include <iostream>
#include "procesos/procesos.h"
#include "hilos/hilos.h"
#include "memoria/memoria.h"

using namespace std;
int main(){
    int opcion;

    do {
        cout << "\n======================================\n";
        cout << "               SISTEMA MINISO          \n";
        cout << "======================================\n";
        cout << "1. Gestion de procesos\n";
        cout << "2. Gestion de hilos y concurrencia\n";
        cout << "3. Gestion de memoria: Particionamiento (First-Fit)\n";
        cout << "4. Gestion de memoria: Paginacion (Marcos y Tablas)\n";
        cout << "0. Salir\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;
        
        switch(opcion){
            case 1:
                modulo_procesos();
                break;
            case 2: 
                cout << "\nModulo de hilos en desarrollo.\n";
                break;
            case 3: 
                ejecutarParticionamiento();
                break;
            case 4: 
                cout << "\nModulo de memoeia en desarrollo.\n";
                break;
            case 0:
                cout << "Cerrando MiniSO...\n";
                break;
            default:
                cout << "\nOpcion invalida.\n";
        }

    }while(opcion != 0);

    return 0;
}
