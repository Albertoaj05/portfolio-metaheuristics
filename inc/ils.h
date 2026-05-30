#pragma once

#include <mh.h>
#include "localsearch.h"

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementación de ILS con Búsqueda Local para el problema de Portfolio
 *  - Parte de una solución inicial mejorada con BL
 *  - Muta la mejor solución encontrada
 *  - Aplica BL tras cada mutación
 *  - Conserva siempre la mejor solución global
 *
 * @see MH
 * @see LocalSearch
 */
class ILS : public MHDouble {

private:
    int iteraciones;       // número de aplicaciones de BL
    int evaluaciones_bl;   // evaluaciones máximas de BL por aplicación
    double porcentaje_mutacion; // porcentaje de pesos que se barajan

    LocalSearch bl;        // búsqueda local usada tras cada mutación

    /**
     * Muta una solución reordenando un porcentaje de sus pesos
     */
    void mutar(tSolution<double>& solucion);

public:
    /**
     * Constructor:
     *  - iteraciones: número de aplicaciones de BL
     *  - evaluaciones_bl: evaluaciones máximas por BL
     *  - porcentaje: porcentaje de pesos que se barajan
     */
    ILS(int iteraciones = 5, int evaluaciones_bl = 2000, double porcentaje = 0.2);

    /**
     * Destructor
     */
    virtual ~ILS() {}

    /**
     * Permite cambiar los parámetros del algoritmo sin recompilar
     */
    void setParametros(int iteraciones, int evaluaciones_bl);

    /**
     * Ejecuta ILS con BL sobre el problema
     */
    virtual ResultMH<double> optimize(Problem<double>& problema, int max_evaluaciones) override;
};
