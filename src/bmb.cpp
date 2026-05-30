#include "../inc/bmb.h"

#include <cassert>

using namespace std;

// Constructor de BMB: guarda el número de arranques y el presupuesto de BL por arranque
BMB::BMB(int iteraciones, int n_evaluaciones_bl) {
    this->iteraciones = iteraciones;
    this->n_evaluaciones_bl = n_evaluaciones_bl;
}

// Permite cambiar los parámetros sin recompilar
void BMB::setParametros(int iteraciones, int n_evaluaciones_bl) {
    this->iteraciones = iteraciones;
    this->n_evaluaciones_bl = n_evaluaciones_bl;
}

// Ejecuta la Búsqueda Multiarranque Básica
ResultMHDouble BMB::optimize(ProblemDouble& problema, int max_evaluaciones) {

    // Comprobamos que el número de evaluaciones sea válido
    assert(max_evaluaciones > 0);

    tSolution<double> mejor_solucion;
    double mejor_fitness = -1e18;
    int evaluaciones = 0;

    // BMB: varios arranques independientes
    for (int i = 0; i < iteraciones; i++) {

        // Generamos una solución inicial aleatoria
        tSolution<double> solucion_aleatoria = problema.createSolution();

        // Aplicamos BL desde esa solución, con 2000 evaluaciones como máximo
        ResultMHDouble resultado = bl.optimizeFromSolution(problema, solucion_aleatoria, n_evaluaciones_bl);

        // Acumulamos las evaluaciones usadas por esta BL
        evaluaciones += resultado.evaluations;

        // Guardamos la mejor solución encontrada entre todos los arranques
        if (resultado.fitness > mejor_fitness) {
            mejor_fitness = resultado.fitness;
            mejor_solucion = resultado.solution;
        }
    }

    return ResultMHDouble(mejor_solucion, mejor_fitness, evaluaciones);
}
