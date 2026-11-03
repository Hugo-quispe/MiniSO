#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include "../procesos/procesos.h"

using namespace std;

void modulo_procesos(void) {
    cout << "\n--- MODULO I: GESTION DE PROCESOS ---" << endl;
    
    pid_t pid = fork();

    if (pid < 0) {
        // Control de error en la creacion del proceso
        cerr << "[Error] No fue posible crear el proceso hijo mediante fork()." << endl;
        return;
    } else if (pid == 0) {
        // Bloque del Proceso Hijo
        cout << "[Hijo] PID actual: " << getpid() << " | PPID (Padre): " << getppid() << endl;
        cout << "[Hijo] Ejecutando programa externo (/bin/ls)..." << endl;
        
        // Ejecucion usando execl
        execl("/bin/ls", "ls", "-l", (char *)NULL);
        
        // Si execl retorna, significa que ocurrio un error
        cerr << "[Error] Fallo al ejecutar el comando con execl." << endl;
        exit(EXIT_FAILURE);
    } else {
        // Bloque del Proceso Padre
        cout << "[Padre] PID: " << getpid() << " | Proceso hijo creado con PID: " << pid << endl;
        cout << "[Padre] Esperando la finalizacion del proceso hijo..." << endl;

        int estado;
        pid_t resultado_wait = waitpid(pid, &estado, 0);

        if (resultado_wait < 0) {
            cerr << "[Error] Fallo al esperar al proceso hijo con waitpid." << endl;
        } else {
            // Verificacion del estado de finalizacion
            if (WIFEXITED(estado)) {
                cout << "[Padre] El proceso hijo (PID: " << resultado_wait 
                     << ") termino normalmente con codigo de salida: " << WEXITSTATUS(estado) << endl;
            } else {
                cout << "[Padre] El proceso hijo finalizo de forma anormal." << endl;
            }
        }
    }
}