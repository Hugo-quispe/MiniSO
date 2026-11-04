#ifndef HILOS_H
#define HILOS_H

// ============================================================================
// MiniSO - Modulo 2: Gestion de Hilos y Concurrencia
//
// Implementa el problema productor-consumidor con hilos reales (std::thread,
// que en Linux se apoya en Pthreads), un buffer compartido de capacidad
// limitada, std::mutex y std::condition_variable.
//
// Punto de entrada para el menu principal de MiniSO:
//     menuHilos();
//
// Compilacion: g++ -Wall -Wextra -std=c++17 -pthread
// ============================================================================

// Muestra el menu del modulo de hilos y concurrencia.
// Regresa cuando el usuario elige "Volver al menu principal".
void menuHilos();

#endif // HILOS_H