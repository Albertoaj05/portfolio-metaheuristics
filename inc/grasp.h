#pragma once

#include <mh.h>
#include "localsearch.h"

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementacion de GRASP para el problema de Portfolio.
 *  - Construye soluciones semi-voraces usando la misma heuristica que Greedy
 *  - En cada paso elige aleatoriamente entre los mejores candidatos de una LRC
 *  - Aplica BL desde cada solucion construida
 *  - Devuelve la mejor solucion encontrada
 *
 * @see MH
 * @see LocalSearch
 */
class GRASP : public MHDouble {

private:
    int iteraciones;       // numero de construcciones semi-voraces
    int evaluaciones_bl;   // evaluaciones maximas de BL por construccion
    int tam_lrc;           // tamano de la lista restringida de candidatos

    LocalSearch bl;        // busqueda local aplicada tras cada construccion

    /**
     * Construye una solucion semi-voraz usando una lista restringida de candidatos
     */
    tSolution<double> construirSolucion(ProblemDouble& problema);

public:
    /**
     * Constructor:
     *  - iteraciones: numero de construcciones semi-voraces
     *  - evaluaciones_bl: evaluaciones maximas de BL por construccion
     *  - tam_lrc: tamano de la lista restringida de candidatos
     */
    GRASP(int iteraciones = 5, int evaluaciones_bl = 2000, int tam_lrc = 5);

    /**
     * Destructor
     */
    virtual ~GRASP() {}

    /**
     * Permite cambiar los parametros del algoritmo sin recompilar
     */
    void setParametros(int iteraciones, int evaluaciones_bl, int tam_lrc);

    /**
     * Ejecuta GRASP sobre el problema
     */
    virtual ResultMH<double> optimize(Problem<double>& problema, int max_evaluaciones) override;
};