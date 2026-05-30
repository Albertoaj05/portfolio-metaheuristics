#include "../inc/simulatedannealing.h"
#include "../inc/portfolio.h"
#include "../common/random.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>
#include <utility>

using namespace std;

// Constructor con los parámetros del guion
SimulatedAnnealing::SimulatedAnnealing(double phi, double mu, double tf) {
    this->phi = phi;
    this->mu = mu;
    this->temperatura_final = tf;
}

// Permite cambiar los parámetros sin recompilar
void SimulatedAnnealing::setParametros(double phi, double mu, double tf) {
    this->phi = phi;
    this->mu = mu;
    this->temperatura_final = tf;
}

// Genera todos los pares (i,j) con i != j que forman el entorno de la solución
vector<pair<int,int>> SimulatedAnnealing::generarEntorno(size_t n) {
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

// Genera un vecino moviendo peso de i hacia j
bool SimulatedAnnealing::generarVecino(const tSolution<double>& actual, tSolution<double>& vecino, int i, int j, double ratio, double lo, double hi) {

    // Si i no tiene nada que ceder, no hay vecino
    if (actual[i] <= 0.0) return false;

    // Cantidad máxima que se quiere mover
    double cantidad = ratio * actual[i];

    // Si j no puede recibir tanto, movemos solo lo que quepa hasta hi
    cantidad = min(cantidad, hi - actual[j]);

    // Evitamos dejar i con un peso positivo menor que lo
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

// Ejecuta ES desde una solución dada. Se reutiliza en ILS-ES
ResultMHDouble SimulatedAnnealing::optimizeFromSolution(ProblemDouble& problema, const tSolution<double>& solucion_inicial, int max_evaluaciones) {
    
    // Comprueba que el número de evaluaciones es positivo
    assert(max_evaluaciones > 0);

    // Para acceder a los datos específicos del problema de portfolio
    Portfolio& portfolio = dynamic_cast<Portfolio&>(problema);

    double lo = portfolio.getLo();
    double hi = portfolio.getHi();
    double ratio = 0.4;

    int m = (int)problema.getSolutionSize();

    // Generamos la solucion actual a partir de la solución inicial dada
    tSolution<double> actual = solucion_inicial;

    // Evaluamos la solución inicial
    double fitness_actual = problema.fitness(actual);
    int evaluaciones = 1;

    tSolution<double> mejor_sol = actual;
    double mejor_fitness = fitness_actual;

    int max_vecinos = 10 * m;
    int max_exitos = m;

    // Número de enfriamientos, la formula es M = maxevals / max_vecinos, 
    // pero aseguramos al menos 1 para evitar división por cero
    int enfriamientos = max(1, max_evaluaciones / max_vecinos);

    // Temperatura inicial del guion -> T0 = mu · Coste(S0) / -ln(phi)
    // Usamos el valor absoluto de fitness_actual para evitar problemas con valores negativos, ya que debe ser un valor positivo.
    double t0 = (mu * fabs(fitness_actual)) / (-log(phi));
    double tf = temperatura_final;

    // Aseguramos Tf < T0
    if (tf >= t0) {
        tf = t0 * 0.001;
    }

    // Enfriamiento de Cauchy modificado.
    double beta = (t0 - tf) / (enfriamientos * t0 * tf);
    double temperatura = t0;

    // Entorno de movimientos posibles.
    vector<pair<int,int>> pares = generarEntorno(m);

    // Bucle externo de enfriamiento, el criterio de parada es cuando se alcanza el número máximo de evaluaciones o
    // bien cuando el número de éxitos en el enfriamiento actual sea igual a 0.
    while (evaluaciones < max_evaluaciones) {
        int vecinos = 0;
        int exitos = 0;

        // Movimientos aleatorios sin repetición
        Random::shuffle(pares.begin(), pares.end());
        size_t pos_par = 0;

        // Bucle interno L(T)
        while ((exitos < max_exitos) && (vecinos < max_vecinos) && (evaluaciones < max_evaluaciones)) {

            // Si se agotan los pares, se vuelven a barajar para seguir generando vecinos
            if (pos_par >= pares.size()) {
                Random::shuffle(pares.begin(), pares.end());
                pos_par = 0;
            }

            // Seleccionamos el siguiente movimiento (i,j) del entorno
            int i = pares[pos_par].first;
            int j = pares[pos_par].second;
            pos_par++;

            tSolution<double> vecino;
            vecinos++;

            // Si no es factible, probamos otro movimiento
            if (!generarVecino(actual, vecino, i, j, ratio, lo, hi)) {
                continue;
            }

            // Evaluamos el vecino factible
            double fitness_vecino = problema.fitness(vecino);
            double delta = fitness_actual - fitness_vecino;

            evaluaciones++;

            bool aceptar = false;

            if (delta < 0.0) aceptar = true;
            else aceptar = Random::get<double>(0.0, 1.0) <= exp(-delta / temperatura);

            if (aceptar) {
                actual = vecino;
                fitness_actual = fitness_vecino;
                exitos++;

                // Guardamos siempre la mejor global
                if (fitness_actual > mejor_fitness) {
                    mejor_fitness = fitness_actual;
                    mejor_sol = actual;
                }
            }
        }

        // Parada: ningún vecino aceptado
        if (exitos == 0) {
            break;
        }

        // Cauchy modificado
        temperatura = temperatura / (1.0 + beta * temperatura);
    }

    return ResultMHDouble(mejor_sol, mejor_fitness, evaluaciones);
}

// ES normal: crea una solución aleatoria y aplica ES desde ella
ResultMHDouble SimulatedAnnealing::optimize(ProblemDouble& problema, int max_evaluaciones) {

    // Comprueba que el número de evaluaciones es positivo
    assert(max_evaluaciones > 0);

    tSolution<double> solucion_inicial = problema.createSolution();

    return optimizeFromSolution(problema, solucion_inicial, max_evaluaciones);
}
