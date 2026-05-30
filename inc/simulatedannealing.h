#pragma once

#include <mh.h>
#include <vector>
#include <utility>

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementación del Enfriamiento Simulado (ES) para el problema de Portfolio
 *  - Parte de una solución inicial aleatoria o de una solución dada
 *  - Genera vecinos moviendo peso entre dos posiciones
 *  - Acepta mejoras y también empeoramientos según la temperatura
 *  - Usa los parámetros phi, mu y temperatura final del guion
 *
 * @see MH
 * @see Problem
 */
class SimulatedAnnealing : public MHDouble {

private:
    double phi;                  // probabilidad inicial de aceptar una solución un 1% peor
    double mu;                   // parámetro usado para calcular la temperatura inicial
    double temperatura_final;    // temperatura final del algoritmo

    /**
     * Genera todos los pares (i, j) con i != j que forman el entorno
     */
    std::vector<std::pair<int,int>> generarEntorno(size_t n);

    /**
     * Genera un vecino moviendo un porcentaje del peso de i hacia j
     * y comprueba si cumple los límites del problema
     */
    bool generarVecino(const tSolution<double>& actual, tSolution<double>& vecino, int i, int j, double ratio, double lo, double hi);

public:
    /**
     * Constructor:
     *  - phi: probabilidad inicial de aceptar una solución un 1% peor
     *  - mu: parámetro de la temperatura inicial
     *  - tf: temperatura final
     */
    SimulatedAnnealing(double phi = 0.3, double mu = 0.2, double tf = 1e-3);

    /**
     * Destructor
     */
    virtual ~SimulatedAnnealing() {}

    /**
     * Permite cambiar los parámetros del ES sin recompilar
     */
    void setParametros(double phi, double mu, double tf);

    /**
     * Ejecuta ES desde una solución inicial aleatoria
     */
    virtual ResultMH<double> optimize(Problem<double>& problema, int max_evaluaciones) override;

    /**
     * Ejecuta ES desde una solución inicial concreta
     */
    ResultMH<double> optimizeFromSolution( Problem<double>& problema, const tSolution<double>& solucion_inicial, int max_evaluaciones);
};
