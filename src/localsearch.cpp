#include <cassert>
#include <algorithm>
#include <vector>
#include <utility>
#include "localsearch.h"
#include "random.hpp"
#include "../inc/portfolio.h"

using namespace std;

// Genera todos los pares (i,j) con i != j que forman el entorno de la solución
vector<pair<int,int>> LocalSearch::generarEntorno(int n) {
    vector<pair<int,int>> pares;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j) {
                pares.push_back({i, j});
            }
        }
    }

    return pares;
}

// Genera un vecino moviendo un porcentaje del peso de i hacia j
bool LocalSearch::generarVecino(const tSolution<double>& actual,
                                tSolution<double>& vecino,
                                int i, int j,
                                double ratio,
                                double lo,
                                double hi) {

    // Creamos el vecino a partir de la solución actual
    vecino = actual;

    // Aplicamos el operador de cambio: quitamos el 40% del valor de i y se lo añadimos a j
    double cantidad = ratio * vecino[i];
    vecino[i] -= cantidad;
    vecino[j] += cantidad;

    // Solo comprobamos los límites, ya que la suma se mantiene
    bool valido = true;

    if (vecino[i] != 0.0 && (vecino[i] < lo || vecino[i] > hi)) {
        valido = false;
    }
    if (vecino[j] != 0.0 && (vecino[j] < lo || vecino[j] > hi)) {
        valido = false;
    }

    return valido;
}

// Función que aplica una búsqueda local primero mejor, explorando vecinos generados al mover un 40% del peso entre dos posiciones
ResultMHDouble LocalSearch::optimize(ProblemDouble &problem, int n_evaluaciones) {

    // Comprobamos que el número de evaluaciones sea válido
    assert(n_evaluaciones > 0);

    // Convertimos el problema general al problema Portfolio
    Portfolio& portfolio = dynamic_cast<Portfolio&>(problem);

    double lo = portfolio.getLo();
    double hi = portfolio.getHi();
    double ratio = 0.4; // porcentaje que se mueve de una posición a otra
    int n = (int)problem.getSolutionSize();

    // Generamos una solución inicial aleatoria
    tSolution<double> actual = problem.createSolution();
    double fitness_actual = problem.fitness(actual);
    int evaluaciones = 1;

    // Repetimos mientras queden evaluaciones disponibles
    while (evaluaciones < n_evaluaciones) {

        // Generamos todos los pares (i,j) con i != j
        vector<pair<int, int>> pares = generarEntorno(n);

        // Barajamos los pares aleatoriamente
        shuffle(pares.begin(), pares.end(), Random::get_engine());

        bool mejora = false;

        // Recorremos los vecinos en orden aleatorio
        for (int k = 0; k < (int)pares.size() && evaluaciones < n_evaluaciones; k++) {
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
                break;
            }
        }

        // Si no se ha encontrado ninguna mejora, se termina la búsqueda
        if (!mejora) {
            break;
        }
    }

    return ResultMHDouble(actual, fitness_actual, evaluaciones);
}