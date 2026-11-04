#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include "../procesos/procesos.h"

using namespace std;

void modulo_procesos(void) {
    cout << "\n--- MODULO I: GESTION DE PROCESOS ---" << endl;
    // Crear el PRIMER proceso hijo
    pid_t pid1 = fork();

    if (pid < 0) {
        // Control de error en la creacion del proceso
        cerr << "[Error] No fue posible crear el primer proceso hijo." << endl;
        return;
    } else if (pid == 0) {
        // Bloque del Primer hijo
        cout << "[Hijo 1] PID actual: " << getpid() << " | PPID (Padre): " << getppid() << endl;
        cout << "[Hijo 1] Ejecutando programa externo (/bin/ls)..." << endl;
        
        // Ejecucion usando execl
        execl("/bin/ls", "ls", "-l", (char *)NULL);
        
        // Si execl retorna, significa que ocurrio un error
        cerr << "[Error] Fallo al ejecutar el comando con execl." << endl;
        exit(EXIT_FAILURE);
    }

    // Crear el SEGUNDO proceso hijo (Ejecucion concurrente)
    pid_t pid2 = fork();

    if (pid2 < 0) {
        cerr << "[Error] No fue posible crear el segundo proceso hijo." << endl;
        // Nota: El primer hijo ya esta corriendo, el padre continua para esperarlo
    } else if (pid2 == 0) {
        // --- Bloque del Segundo Hijo ---
        cout << "[Hijo 2] PID actual: " << getpid() << " | PPID (Padre): " << getppid() << endl;
        cout << "[Hijo 2] Ejecutando comando externo (/bin/date)..." << endl;
        execl("/bin/date", "date", (char *)NULL);
        
        cerr << "[Error] Fallo al ejecutar execl en Hijo 2." << endl;
        exit(EXIT_FAILURE);
    }

    // Bloque del Proceso Padre
    cout << "[Padre] PID: " << getpid() << " | Hijos creados -> Hijo 1: " << pid1 
        << ", Hijo 2: " << pid2 << endl;
    cout << "[Padre] Esperando la finalizacion de ambos procesos hijos de forma concurrente..." << endl;

    int estado;
    pid_t pid_terminado;
    // Esperamos a los dos hijos conforme vayan terminando usando un bucle wait
    for (int i = 0; i < 2; i++) {
        // wait(NULL) o waitpid(-1, &estado, 0) espera a cualquier hijo disponible
        pid_terminado = waitpid(-1, &estado, 0);

        if (pid_terminado < 0) {
            cerr << "[Error] Fallo al esperar a un proceso hijo." << endl;
        } else {
            // Verificacion del estado de finalizacion de cada uno
            if (WIFEXITED(estado)) {
                cout << "[Padre] El proceso hijo (PID: " << pid_terminado 
                     << ") termino normalmente con codigo de salida: " << WEXITSTATUS(estado) << endl;
            } else {
                cout << "[Padre] El proceso hijo (PID: " << pid_terminado 
                     << ") finalizo de forma anormal." << endl;
            }
        }
    }
}