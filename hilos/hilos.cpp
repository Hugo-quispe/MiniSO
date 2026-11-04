// ============================================================================
// MiniSO - Modulo 2: Gestion de Hilos y Concurrencia
// Problema productor-consumidor con hilos reales, mutex y variables de condicion.
// ============================================================================

#include "hilos.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace {

// Constantes del modulo
constexpr int NUM_PRODUCTORES = 2;     // minimo exigido: 2
constexpr int NUM_CONSUMIDORES = 2;    // minimo exigido: 2
constexpr int MAX_ELEMENTOS = 10000;   // limite razonable para la consola
constexpr int MAX_CAPACIDAD = 1000;

// Configuracion de una simulacion

// El perfil de velocidad solo simula cuanto "trabajo" hace cada hilo al
// generar o procesar un elemento. NO sustituye a la sincronizacion: el
// resultado es correcto con cualquier perfil (incluido el de 0 ms).
struct PerfilVelocidad {
    string nombre;
    int msProducir;
    int msConsumir;
};

struct Configuracion {
    int totalElementos;
    size_t capacidad;
    PerfilVelocidad perfil;
};

vector<PerfilVelocidad> perfilesDisponibles() {
    return {
        {"Equilibrada (productores y consumidores a la par)", 15, 15},
        {"Consumidores lentos (el buffer tiende a llenarse)", 5, 60},
        {"Productores lentos (el buffer tiende a vaciarse)", 60, 5},
        {"Sin pausas (maxima velocidad, prueba de estres)", 0, 0},
    };
}

// Estado compartido por TODOS los hilos de una simulacion
struct Simulacion {
    explicit Simulacion(const Configuracion& c)
        : config(c), vecesConsumido(c.totalElementos + 1, 0) {}

    const Configuracion config;

    // --- Buffer compartido y su sincronizacion ---
    queue<int> buffer;                       // protegido por mtx
    mutex mtx;                               // exclusion mutua sobre buffer y contadores
    condition_variable puedeProducir;        // "hay al menos un espacio libre"
    condition_variable puedeConsumir;        // "hay al menos un elemento" o "ya no habra mas"
    bool produccionTerminada = false;        // protegido por mtx
    bool cancelada = false;                  // protegido por mtx (fallo al crear hilos)

    // Reparte numeros de elemento unicos (1..totalElementos) entre productores.
    // Es atomico: dos productores nunca reciben el mismo numero.
    atomic<int> siguienteElemento{1};

    // Registro para estadisticas y validacion (protegido por mtx)
    int producidos = 0;                      // elementos insertados en el buffer
    int consumidos = 0;                      // elementos retirados del buffer
    size_t maxOcupacion = 0;
    long esperasProductor = 0;               // veces que un productor tuvo que dormir
    long esperasConsumidor = 0;              // veces que un consumidor tuvo que dormir
    vector<int> vecesConsumido;              // vecesConsumido[n] = cuantas veces salio n

    // salida por consola
    // Orden de adquisicion SIEMPRE: mtx -> mtxSalida (nunca al reves),
    // asi no puede haber interbloqueo entre ambos candados.
    mutex mtxSalida;
};

struct ResultadoSimulacion {
    int producidos = 0;
    int consumidos = 0;
    int pendientes = 0;
    int perdidos = 0;
    int duplicados = 0;
    size_t maxOcupacion = 0;
    long esperasProductor = 0;
    long esperasConsumidor = 0;
    bool cancelada = false;
    bool correcto = false;
};

// Utilidades

// Imprime una linea completa sin que se mezcle con la de otro hilo.
void imprimir(Simulacion& s, const string& mensaje) {
    lock_guard<mutex> guardia(s.mtxSalida);
    cout << mensaje << endl;
}

// Texto "[buffer 2/5]". Debe llamarse con s.mtx tomado (lee el buffer).
string estadoBuffer(const Simulacion& s) {
    return "[buffer " + to_string(s.buffer.size()) + "/" +
           to_string(s.config.capacidad) + "]";
}

// Simula el tiempo de trabajo de un hilo (generar o procesar un elemento).
void simularTrabajo(int milisegundos) {
    if (milisegundos > 0) this_thread::sleep_for(chrono::milliseconds(milisegundos));
}

// ---------------------------------------------------------------------------
// PRODUCTOR
//   1. Genera un elemento.            2. Solicita acceso al buffer (mutex).
//   3. Espera si el buffer esta lleno. 4. Inserta el elemento.
//   5. Notifica a los consumidores.
// ---------------------------------------------------------------------------
void productor(Simulacion& s, int id) {
    const string nombre = "Productor " + to_string(id);

    while (true) {
        // 1. Generar: se reclama un numero unico. Si ya se repartieron todos, termina.
        const int elemento = s.siguienteElemento.fetch_add(1);
        if (elemento > s.config.totalElementos) break;

        imprimir(s, "[" + nombre + "] genera el elemento " + to_string(elemento));
        simularTrabajo(s.config.perfil.msProducir);

        // 2. Solicitar acceso exclusivo al buffer.
        unique_lock<mutex> lock(s.mtx);

        // 3. Esperar mientras no haya espacio. wait() suelta el mutex mientras
        //    duerme y lo recupera al despertar; el predicado se reevalua siempre
        //    (protege contra despertares espurios y contra otro productor que
        //    se haya adelantado).
        if (s.buffer.size() >= s.config.capacidad && !s.cancelada) {
            ++s.esperasProductor;
            imprimir(s, "[" + nombre + "] BUFFER LLENO " + estadoBuffer(s) +
                            " -> espera para insertar el elemento " + to_string(elemento));
        }
        s.puedeProducir.wait(lock, [&] {
            return s.buffer.size() < s.config.capacidad || s.cancelada;
        });
        if (s.cancelada) {
            imprimir(s, "[" + nombre + "] simulacion cancelada, abandona el elemento " +
                            to_string(elemento));
            break;   // el unique_lock se libera al salir del bloque
        }

        // 4. Insertar (seccion critica).
        s.buffer.push(elemento);
        ++s.producidos;
        if (s.buffer.size() > s.maxOcupacion) s.maxOcupacion = s.buffer.size();
        imprimir(s, "[" + nombre + "] INSERTA el elemento " + to_string(elemento) + " " +
                        estadoBuffer(s));

        // 5. Soltar el mutex y avisar: hay un elemento nuevo para un consumidor.
        lock.unlock();
        s.puedeConsumir.notify_one();
    }

    imprimir(s, "[" + nombre + "] FINALIZA");
}

// ---------------------------------------------------------------------------
// CONSUMIDOR
//   1. Solicita acceso al buffer.   2. Espera si esta vacio.
//   3. Extrae un elemento.          4. Lo procesa.
//   5. Notifica a los productores.
// Termina solo cuando el buffer esta vacio Y ya no habra mas produccion:
// asi siempre vacia el buffer antes de salir y nunca se queda dormido para
// siempre.
// ---------------------------------------------------------------------------
void consumidor(Simulacion& s, int id) {
    const string nombre = "Consumidor " + to_string(id);

    while (true) {
        // 1. Solicitar acceso exclusivo al buffer.
        unique_lock<mutex> lock(s.mtx);

        // 2. Esperar mientras no haya nada que retirar Y todavia pueda llegar algo.
        if (s.buffer.empty() && !s.produccionTerminada) {
            ++s.esperasConsumidor;
            imprimir(s, "[" + nombre + "] BUFFER VACIO " + estadoBuffer(s) + " -> espera");
        }
        s.puedeConsumir.wait(lock, [&] {
            return !s.buffer.empty() || s.produccionTerminada;
        });

        // Condicion de salida: se desperto por fin de produccion y no queda nada.
        if (s.buffer.empty()) break;

        // 3. Extraer (seccion critica).
        const int elemento = s.buffer.front();
        s.buffer.pop();
        ++s.consumidos;
        ++s.vecesConsumido[elemento];
        imprimir(s, "[" + nombre + "] RETIRA el elemento " + to_string(elemento) + " " +
                        estadoBuffer(s));

        // 5. Soltar el mutex y avisar: hay un espacio libre para un productor.
        lock.unlock();
        s.puedeProducir.notify_one();

        // 4. Procesar fuera de la seccion critica (no bloquea a los demas hilos).
        simularTrabajo(s.config.perfil.msConsumir);
        imprimir(s, "[" + nombre + "] termino de procesar el elemento " + to_string(elemento));
    }

    imprimir(s, "[" + nombre + "] FINALIZA (no quedan elementos por consumir)");
}

// ---------------------------------------------------------------------------
// Ejecucion de una simulacion completa
// ---------------------------------------------------------------------------

// Une (join) todos los hilos de un vector que sigan activos.
void unirHilos(vector<thread>& hilos) {
    for (auto& h : hilos)
        if (h.joinable()) h.join();
}

ResultadoSimulacion ejecutarSimulacion(const Configuracion& config) {
    Simulacion s(config);
    vector<thread> productores, consumidores;

    cout << "\n=== INICIO DE LA SIMULACION ===\n"
         << "Productores: " << NUM_PRODUCTORES << " | Consumidores: " << NUM_CONSUMIDORES
         << " | Capacidad del buffer: " << config.capacidad
         << " | Elementos a producir: " << config.totalElementos << "\n"
         << "Velocidad: " << config.perfil.nombre << "\n"
         << "--------------------------------\n";

    // Creacion de hilos. Si el sistema no puede crear alguno (system_error),
    // se cancela la simulacion: se despierta a todos para que ningun hilo ya
    // creado quede bloqueado, y luego se hace join de lo que exista.
    try {
        consumidores.reserve(NUM_CONSUMIDORES);
        productores.reserve(NUM_PRODUCTORES);
        for (int i = 1; i <= NUM_CONSUMIDORES; ++i)
            consumidores.emplace_back(consumidor, ref(s), i);
        for (int i = 1; i <= NUM_PRODUCTORES; ++i)
            productores.emplace_back(productor, ref(s), i);
    } catch (const exception& e) {
        cerr << "ERROR al crear hilos: " << e.what() << "\n"
             << "Se cancela la simulacion y se liberan los hilos ya creados.\n";
        {
            lock_guard<mutex> guardia(s.mtx);
            s.cancelada = true;
            s.produccionTerminada = true;
        }
        s.puedeProducir.notify_all();
        s.puedeConsumir.notify_all();
    }

    // Fase 1: esperar a que TODOS los productores terminen.
    unirHilos(productores);

    // Fase 2: avisar a los consumidores de que no habra mas produccion.
    // El cambio se hace con el mutex tomado y se usa notify_all para que
    // ningun consumidor dormido pierda el aviso.
    {
        lock_guard<mutex> guardia(s.mtx);
        s.produccionTerminada = true;
    }
    s.puedeConsumir.notify_all();

    // Fase 3: los consumidores vacian el buffer y terminan.
    unirHilos(consumidores);

    imprimir(s, "--------------------------------\n=== SIMULACION FINALIZADA: todos los hilos terminaron ===");

    // Validacion. Ya no hay hilos activos, por lo que leer el estado es seguro.
    ResultadoSimulacion r;
    r.producidos = s.producidos;
    r.consumidos = s.consumidos;
    r.pendientes = static_cast<int>(s.buffer.size());
    r.maxOcupacion = s.maxOcupacion;
    r.esperasProductor = s.esperasProductor;
    r.esperasConsumidor = s.esperasConsumidor;
    r.cancelada = s.cancelada;
    for (int n = 1; n <= config.totalElementos; ++n) {
        if (s.vecesConsumido[n] == 0) ++r.perdidos;
        else if (s.vecesConsumido[n] > 1) r.duplicados += s.vecesConsumido[n] - 1;
    }
    r.correcto = !r.cancelada &&
                 r.producidos == config.totalElementos &&
                 r.consumidos == config.totalElementos &&
                 r.pendientes == 0 && r.perdidos == 0 && r.duplicados == 0 &&
                 r.maxOcupacion <= config.capacidad;
    return r;
}

void mostrarResumen(const Configuracion& config, const ResultadoSimulacion& r) {
    cout << "\n================ RESUMEN ================\n"
         << "Capacidad del buffer            : " << config.capacidad << "\n"
         << "Ocupacion maxima alcanzada      : " << r.maxOcupacion << "\n"
         << "Veces que un productor espero   : " << r.esperasProductor << " (buffer lleno)\n"
         << "Veces que un consumidor espero  : " << r.esperasConsumidor << " (buffer vacio)\n"
         << "-----------------------------------------\n"
         << "Elementos a producir (config)   : " << config.totalElementos << "\n"
         << "Elementos producidos            : " << r.producidos << "\n"
         << "Elementos consumidos            : " << r.consumidos << "\n"
         << "Elementos pendientes en buffer  : " << r.pendientes << "\n"
         << "Elementos perdidos              : " << r.perdidos << "\n"
         << "Elementos duplicados            : " << r.duplicados << "\n"
         << "-----------------------------------------\n";
    if (r.cancelada)
        cout << "Resultado: SIMULACION CANCELADA (error al crear hilos)\n";
    else
        cout << "Validacion (sin perdidas ni duplicaciones): "
             << (r.correcto ? "CORRECTO" : "INCORRECTO") << "\n";
    cout << "=========================================\n";
}

void correrYMostrar(const Configuracion& config) {
    mostrarResumen(config, ejecutarSimulacion(config));
}

// Lectura validada desde teclado

// Pide un entero en [minimo, maximo]. Repite hasta recibir un valor valido.
// Devuelve false si la entrada se cerro (Ctrl+D / fin de archivo).
bool leerEntero(const string& pregunta, int minimo, int maximo, int& valor) {
    string linea;
    while (true) {
        cout << pregunta << flush;
        if (!getline(cin, linea)) return false;

        size_t pos = 0;
        long long numero = 0;
        try {
            numero = stoll(linea, &pos);
        } catch (const exception&) {
            cout << "Entrada no valida: escriba un numero entero.\n";
            continue;
        }
        while (pos < linea.size() && isspace(static_cast<unsigned char>(linea[pos]))) ++pos;
        if (pos != linea.size()) {
            cout << "Entrada no valida: escriba solo un numero entero.\n";
            continue;
        }
        if (numero < minimo || numero > maximo) {
            if (minimo == 1)
                cout << "El valor debe ser mayor que cero y no superar " << maximo << ".\n";
            else
                cout << "El valor debe estar entre " << minimo << " y " << maximo << ".\n";
            continue;
        }
        valor = static_cast<int>(numero);
        return true;
    }
}

bool elegirPerfil(PerfilVelocidad& elegido) {
    const auto perfiles = perfilesDisponibles();
    cout << "\nVelocidad de los hilos:\n";
    for (size_t i = 0; i < perfiles.size(); ++i)
        cout << "  " << (i + 1) << ". " << perfiles[i].nombre << "\n";
    int opcion = 0;
    if (!leerEntero("Perfil: ", 1, static_cast<int>(perfiles.size()), opcion)) return false;
    elegido = perfiles[opcion - 1];
    return true;
}

void ejecutarPersonalizada() {
    int elementos = 0, capacidad = 0;
    PerfilVelocidad perfil;
    if (!leerEntero("Cantidad total de elementos a producir (1-" +
                        to_string(MAX_ELEMENTOS) + "): ", 1, MAX_ELEMENTOS, elementos)) return;
    if (!leerEntero("Capacidad del buffer (1-" + to_string(MAX_CAPACIDAD) + "): ",
                    1, MAX_CAPACIDAD, capacidad)) return;
    if (!elegirPerfil(perfil)) return;
    correrYMostrar({elementos, static_cast<size_t>(capacidad), perfil});
}

// Pruebas predefinidas del enunciado.
struct PruebaPredefinida {
    int capacidad;
    int elementos;
    int indicePerfil;   // posicion en perfilesDisponibles()
};
const PruebaPredefinida PRUEBAS[] = {
    {3, 20, 0},   // A: capacidad 3, 20 elementos, equilibrada
    {5, 30, 1},   // B: capacidad 5, 30 elementos, consumidores lentos (buffer lleno)
    {2, 10, 2},   // C: capacidad 2, 10 elementos, productores lentos (buffer vacio)
};

void ejecutarPrueba(const PruebaPredefinida& p) {
    correrYMostrar({p.elementos, static_cast<size_t>(p.capacidad),
                    perfilesDisponibles()[p.indicePerfil]});
}

}  // namespace

// ---------------------------------------------------------------------------
// Menu del modulo (unica funcion publica)
// ---------------------------------------------------------------------------
void menuHilos() {
    while (true) {
        cout << "\n====== GESTION DE HILOS Y CONCURRENCIA ======\n"
             << "1. Simulacion personalizada (elementos, capacidad y velocidad)\n"
             << "2. Prueba A: capacidad 3, 20 elementos\n"
             << "3. Prueba B: capacidad 5, 30 elementos (consumidores lentos)\n"
             << "4. Prueba C: capacidad 2, 10 elementos (productores lentos)\n"
             << "5. Ejecutar las tres pruebas (A, B y C)\n"
             << "0. Volver al menu principal\n";

        int opcion = 0;
        if (!leerEntero("Opcion: ", 0, 5, opcion) || opcion == 0) return;

        switch (opcion) {
            case 1: ejecutarPersonalizada(); break;
            case 2: ejecutarPrueba(PRUEBAS[0]); break;
            case 3: ejecutarPrueba(PRUEBAS[1]); break;
            case 4: ejecutarPrueba(PRUEBAS[2]); break;
            case 5:
                for (const auto& p : PRUEBAS) ejecutarPrueba(p);
                break;
            default: break;
        }
    }
}