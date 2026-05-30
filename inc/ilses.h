#pragma once

#include <mh.h>
#include "simulatedannealing.h"

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementación de ILS hibridado con Enfriamiento Simulado
 *  - Parte de una solución inicial mejorada con ES
 *  - Muta la mejor solución encontrada
 *  - Aplica ES tras cada mutación
 *  - Conserva siempre la mejor solución global
 *
 * @see MH
 * @see SimulatedAnnealing
 */
class ILSES : public MHDouble {

private:
    int iteraciones;       // número de aplicaciones de ES
    int evaluaciones_es;   // evaluaciones máximas de ES por aplicación
    double porcentaje_mutacion; // porcentaje de pesos que se barajan

    SimulatedAnnealing es; // enfriamiento simulado usado tras cada mutación

    /**
     * Muta una solución reordenando un porcentaje de sus pesos
     */
    void mutar(tSolution<double>& sol);

public:
    /**
     * Constructor:
     *  - iteraciones: número de aplicaciones de ES
     *  - evaluaciones_es: evaluaciones máximas por ES
     *  - porcentaje: porcentaje de pesos que se barajan
     */
    ILSES(int iteraciones = 5, int evaluaciones_es = 2000, double porcentaje = 0.2);

    /**
     * Destructor
     */
    virtual ~ILSES() {}

    /**
     * Permite cambiar las iteraciones y evaluaciones de ES
     */
    void setParametros(int iteraciones, int evaluaciones_es);

    /**
     * Permite cambiar los parámetros internos del ES
     */
    void setParametrosES(double phi, double mu, double tf);

    /**
     * Ejecuta ILS-ES sobre el problema
     */
    virtual ResultMH<double> optimize(Problem<double>& problema, int max_evaluaciones) override;
};
