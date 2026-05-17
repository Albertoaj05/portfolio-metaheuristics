#include <cassert>
#include <algorithm>
#include <vector>
#include <utility>
#include "localsearch.h"
#include "random.hpp"
#include "../inc/portfolio.h"

using namespace std;

// Genera todos los pares (i,j) con i != j que forman el entorno de la solución
vector<pair<int,int>> LocalSearch::generarEntorno(size_t n) {
    vector<pair<int,int>> pares;

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            if (i != j) {
                pares.push_back({(int)i, (int)j});
            }
        }
    }

    return pares;
}

// Genera un vecino moviendo un porcentaje del peso de i hacia j, reduciendo la transferencia si es necesario
bool LocalSearch::generarVecino(const tSolution<double>& actual, tSolution<double>& vecino, int i, int j, double ratio, double lo, double hi) {

    // Si i no tiene nada que ceder, no hay vecino
    if (actual[i] <= 0.0) return false;

    // Cantidad máxima que se quiere mover
    double cantidad = ratio * actual[i];

    // Si j no puede recibir tanto, movemos solo lo que quepa hasta hi
    cantidad = min(cantidad, hi - actual[j]);

    // Si i quedaría entre 0 y lo, reducimos la cantidad para dejar i en lo.
    // Así nunca movemos más del ratio indicado.
    if (actual[i] - cantidad > 0.0 && actual[i] - cantidad < lo) {
        cantidad = actual[i] - lo;
    }

    if (cantidad <= 0.0) return false;

    // Si j estaba a 0, al activarlo debe llegar al mínimo permitido
    if (actual[j] == 0.0 && cantidad < lo) return false;

    // Solo ahora, que el vecino es válido, copiamos y aplicamos
    vecino = actual;
    vecino[i] -= cantidad;
    vecino[j] += cantidad;

    return true;
}

// Función que aplica una búsqueda local primero mejor, explorando vecinos generados al mover un 40% del peso entre dos posiciones
ResultMHDouble LocalSearch::optimizar(ProblemDouble &problem, int n_evaluaciones) {

    // Comprobamos que el número de evaluaciones sea válido
    assert(n_evaluaciones > 0);

    // Convertimos el problema general al problema Portfolio
    Portfolio& portfolio = dynamic_cast<Portfolio&>(problem);

    double lo = portfolio.getLo();
    double hi = portfolio.getHi();
    double ratio = 0.4; // porcentaje que se mueve de una posición a otra
    size_t n = problem.getSolutionSize();

    // Generamos una solución inicial aleatoria
    tSolution<double> actual = problem.createSolution();
    double fitness_actual = problem.fitness(actual);
    int evaluaciones = 1;

    // Generamos todos los pares (i,j) con i != j que forman el entorno de la solución
    vector<pair<int,int>> pares = generarEntorno(n);

    // Repetimos mientras queden evaluaciones disponibles
    while (evaluaciones < n_evaluaciones) {
        
        // Barajamos los pares aleatoriamente
        shuffle(pares.begin(), pares.end(), Random::get_engine());
    
        bool mejora = false;

        // Recorremos los vecinos en orden aleatorio
        for (size_t k = 0; k < pares.size() && evaluaciones < n_evaluaciones; k++) {

            int i = pares[k].first;
            int j = pares[k].second;

            // Generamos el vecino
            tSolution<double> vecino;
            bool valido = generarVecino(actual, vecino, i, j, ratio, lo, hi);

            // Si el vecino no es válido, pasamos al siguiente
            if (!valido) continue;

            // Evaluamos el vecino
            double fitness_vecino = problem.fitness(vecino);
            evaluaciones++;

            // si mejora, lo aceptamos y dejamos de recorrer el vecindario
            if (fitness_vecino > fitness_actual) {
                actual = vecino;
                fitness_actual = fitness_vecino;
                mejora = true;
            }

            if (mejora) break;
        }

        // Si no se ha encontrado ninguna mejora, se termina la búsqueda
        if (!mejora) {
            break;
        }
    }

    return ResultMHDouble(actual, fitness_actual, evaluaciones);
}

// Método que llama a ejecutar la búsqueda local
ResultMHDouble LocalSearch::optimize(ProblemDouble &problem, int n_evaluaciones) {
    return optimizar(problem, n_evaluaciones);
}
