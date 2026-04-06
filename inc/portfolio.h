#pragma once

#include "../common/problem.h"
#include <vector>
#include <string>
#include <utility>

using namespace std;

/**
 * Implementación del problema de optimización de cartera (Portfolio)
 *  - Se reparte un capital entre varias empresas
 *  - Se busca maximizar el beneficio y minimizar el riesgo
 *
 * @see Problem
 */
class Portfolio : public Problem<double> {

private:
    size_t n; // número de empresas
    size_t T; // número de días
    double lo, hi; // límites de inversión por empresa
    double lambda; // penalización del riesgo

    vector<vector<double>> mu; // retornos [día][empresa]
    vector<double> media_por_accion; // media de retornos por empresa
    vector<vector<double>> cov; // matriz de covarianza
    vector<double> parte_interna_beneficio; // beneficio acumulado (log)

public:

    /**
     * Constructor:
     *  - Lee el fichero de datos
     *  - Calcula medias y matriz de covarianza
     */
    Portfolio(const string& filename, double lo, double hi, double lambda = 500.0);

    /**
     * Devuelve el número de empresas (tamaño de la solución)
     */
    size_t getSolutionSize() override;

    /**
     * Devuelve el rango permitido de cada variable (0, hi)
     */
    pair<double, double> getSolutionDomainRange() override;

    /**
     * Calcula el fitness de una solución:
     *  beneficio - lambda * riesgo
     */
    tFitness fitness(const tSolution<double>& solution) override;

    /**
     * Genera una solución aleatoria válida
     */
    tSolution<double> createSolution() override;

    /**
     * Comprueba si una solución es válida:
     *  - suma ≈ 1
     *  - cada peso es 0 o está en [lo, hi]
     */
    bool isValid(const tSolution<double>& solution) override;

    /**
     * Repara una solución para que cumpla las restricciones
     */
    void fix(tSolution<double>& solution) override;

    // Métodos auxiliares para acceder a datos del problema
    const vector<double>& getMediaPorAccion() const;
    const vector<double>& getBeneficio() const;
    const vector<vector<double>>& getCov() const;
    double getLambda() const;
    double getHi() const;
    size_t getT() const;
    double getLo() const;
    const vector<vector<double>>& getMu() const;
};