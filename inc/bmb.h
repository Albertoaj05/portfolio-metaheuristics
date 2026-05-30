#pragma once

#include <mh.h>
#include "localsearch.h"

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementacion de la Busqueda Multiarranque Basica (BMB).
 *  - Repite varios arranques independientes
 *  - En cada arranque genera una solucion aleatoria
 *  - Aplica BL desde esa solucion
 *  - Devuelve la mejor solucion encontrada entre todos los arranques
 *
 * @see MH
 * @see LocalSearch
 */
class BMB : public MHDouble {

private:
    int iteraciones;          // numero de arranques independientes
    int n_evaluaciones_bl;    // evaluaciones maximas de BL por arranque
    LocalSearch bl;           // busqueda local aplicada en cada arranque

public:
    /**
     * Constructor:
     *  - iteraciones: numero de arranques independientes
     *  - n_evaluaciones_bl: evaluaciones maximas de BL en cada arranque
     */
    BMB(int iteraciones = 5, int n_evaluaciones_bl = 2000);

    /**
     * Destructor
     */
    virtual ~BMB() {}

    /**
     * Permite cambiar los parametros del algoritmo sin recompilar
     */
    void setParametros(int iteraciones, int n_evaluaciones_bl);

    /**
     * Ejecuta la Busqueda Multiarranque Basica
     */
    virtual ResultMH<double> optimize(Problem<double>& problem, int maxevals) override;
};