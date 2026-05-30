#include "../inc/ilses.h"
#include "../common/random.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

using namespace std;

// Constructor con los parámetros del guion
ILSES::ILSES(int iteraciones, int evaluaciones_es, double porcentaje) {
    this->iteraciones = iteraciones;
    this->evaluaciones_es = evaluaciones_es;
    this->porcentaje_mutacion = porcentaje;
}

// Permite cambiar iteraciones y evaluaciones de ES
void ILSES::setParametros(int iteraciones, int evaluaciones_es) {
    this->iteraciones = iteraciones;
    this->evaluaciones_es = evaluaciones_es;
}

// Permite cambiar los parámetros internos del ES
void ILSES::setParametrosES(double phi, double mu, double tf) {
    es.setParametros(phi, mu, tf);
}

// Muta la solución barajando aleatoriamente un porcentaje de sus pesos
void ILSES::mutar(tSolution<double>& solucion) {

    // Número de pesos totales
    int n = (int)solucion.size();

    // Guarda el numero de pesos a mutar.
    int num_mutados = (int)ceil(porcentaje_mutacion * n);

    // Creamos un vector con los indices de los pesos.
    vector<int> indices(n);
    for (int i = 0; i < n; i++) {
        indices[i] = i;
    }

    // Barajamos los indices para elegir posiciones aleatorias.
    Random::shuffle(indices.begin(), indices.end());

    // Nos quedamos solo con los primeros num_mutados indices.
    indices.resize(num_mutados);

    // Vector donde guardamos los valores de las posiciones elegidas.
    vector<double> valores;
    valores.reserve(num_mutados);

    // Guardamos los valores originales de las posiciones elegidas.
    for (int i = 0; i < num_mutados; i++) {
        int idx = indices[i];
        valores.push_back(solucion[idx]);
    }

    // Barajamos esos pesos.
    Random::shuffle(valores.begin(), valores.end());

    // Volvemos a colocar los pesos barajados en las posiciones seleccionadas.
    for (int i = 0; i < num_mutados; i++) {
        solucion[indices[i]] = valores[i];
    }
}

// Ejecuta la hibridación ILS-ES
ResultMHDouble ILSES::optimize(ProblemDouble& problema, int max_evaluaciones) {

    // Comprueba que el número de evaluaciones es positivo
    assert(max_evaluaciones > 0);

    int evaluaciones = 0;

    // Solución inicial + ES
    tSolution<double> inicial = problema.createSolution();
    ResultMHDouble resultado = es.optimizeFromSolution(problema, inicial, evaluaciones_es);
    evaluaciones += resultado.evaluations;

    // Actualizamos la mejor solución encontrada hasta ahora
    tSolution<double> mejor_sol = resultado.solution;
    double mejor_fitness = resultado.fitness;

    // Mutación de la mejor solución + ES       
    for (int i = 1; i < iteraciones; i++) {

        // Mutamos la mejor solución encontrada hasta ahora
        tSolution<double> mutada = mejor_sol;
        mutar(mutada);

        // Aplicamos ES desde la solución mutada, con evaluaciones_es como máximo
        resultado = es.optimizeFromSolution(problema, mutada, evaluaciones_es);
        evaluaciones += resultado.evaluations;

        // Actualizamos la mejor solución encontrada hasta ahora
        if (resultado.fitness > mejor_fitness) {
            mejor_fitness = resultado.fitness;
            mejor_sol = resultado.solution;
        }
    }

    return ResultMHDouble(mejor_sol, mejor_fitness, evaluaciones);
}
