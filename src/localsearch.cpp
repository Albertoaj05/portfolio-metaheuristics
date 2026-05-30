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

// BL desde una solución inicial dada.
ResultMHDouble LocalSearch::optimizeFromSolution(ProblemDouble& problem, const tSolution<double>& inicial, int maxevals) {
    assert(maxevals > 0);

    Portfolio& portfolio = dynamic_cast<Portfolio&>(problem);

    double lo = portfolio.getLo();
    double hi = portfolio.getHi();
    double ratio = 0.4;

    size_t n = problem.getSolutionSize();

    // Partimos de la solución recibida.
    // No hacemos fix porque createSolution ya repara, y la mutación de ILS solo reordena pesos.
    tSolution<double> actual = inicial;

    double fitness_actual = problem.fitness(actual);
    int evaluaciones = 1;

    vector<pair<int,int>> pares = generarEntorno(n);

    while (evaluaciones < maxevals) {

        // Exploración aleatoria del entorno, como en la BL de la P1.
        Random::shuffle(pares.begin(), pares.end());

        bool mejora = false;

        for (size_t k = 0; k < pares.size() && evaluaciones < maxevals; k++) {

            int i = pares[k].first;
            int j = pares[k].second;

            tSolution<double> vecino;

            if (!generarVecino(actual, vecino, i, j, ratio, lo, hi)) {
                continue;
            }

            double fitness_vecino = problem.fitness(vecino);
            evaluaciones++;

            // Primero mejor: aceptamos el primer vecino que mejore.
            if (fitness_vecino > fitness_actual) {
                actual = vecino;
                fitness_actual = fitness_vecino;
                mejora = true;
                break;
            }
        }

        // Si se recorre todo el entorno sin mejorar, estamos en óptimo local.
        if (!mejora) {
            break;
        }
    }

    return ResultMHDouble(actual, fitness_actual, evaluaciones);
}

// BL normal: genera solución aleatoria y reutiliza optimizeFromSolution.
ResultMHDouble LocalSearch::optimize(ProblemDouble& problem, int maxevals) {

    assert(maxevals > 0);

    tSolution<double> inicial = problem.createSolution();

    return optimizeFromSolution(problem, inicial, maxevals);
}