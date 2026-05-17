#pragma once

#include "../common/problem.h"
#include <vector>
#include <string>
#include <utility>

using namespace std;

/**
 * Implementación del problema de Portfolio
 *  - Lee los datos históricos de rentabilidad
 *  - Calcula beneficio acumulado y matriz de covarianza
 *  - Evalúa soluciones mediante beneficio - lambda * riesgo
 *
 * @see Problem
 */
class Portfolio : public Problem<double> {

private:
    size_t n;       // número de empresas
    size_t T;       // número de días
    double lo, hi;  // límites mínimo y máximo de inversión por empresa
    double lambda;  // penalización del riesgo

    vector<vector<double>> mu;              // retornos [día][empresa]
    vector<double> media_por_accion;        // media de retornos por empresa
    vector<vector<double>> cov;             // matriz de covarianza
    vector<double> parte_interna_beneficio; // beneficio acumulado logarítmico por empresa

public:

    /**
     * Constructor:
     *  - Lee el fichero CSV de datos
     *  - Guarda los límites de inversión
     *  - Calcula medias, beneficio acumulado y covarianzas
     */
    Portfolio(const string& filename, double lo, double hi, double lambda = 500.0);

    /**
     * Devuelve el número de empresas, es decir, el tamaño de cada solución
     */
    size_t getSolutionSize() override;

    /**
     * Devuelve el rango general del dominio de cada variable
     */
    pair<double, double> getSolutionDomainRange() override;

    /**
     * Calcula el fitness de una solución:
     *  fitness = beneficio - lambda * riesgo
     */
    tFitness fitness(const tSolution<double>& solution) override;

    /**
     * Genera una solución aleatoria válida del problema
     */
    tSolution<double> createSolution() override;

    /**
     * Comprueba si una solución cumple las restricciones:
     *  - La suma de pesos debe ser aproximadamente 1
     *  - Cada peso debe ser 0 o estar dentro de [lo, hi]
     */
    bool isValid(const tSolution<double>& solution) override;

    /**
     * Repara una solución para que cumpla las restricciones del problema
     */
    void fix(tSolution<double>& solution) override;

    /**
     * Devuelve la media de retornos por empresa
     */
    const vector<double>& getMediaPorAccion() const;

    /**
     * Devuelve la parte interna del beneficio acumulado logarítmico
     */
    const vector<double>& getBeneficio() const;

    /**
     * Devuelve la matriz de covarianza
     */
    const vector<vector<double>>& getCov() const;

    /**
     * Devuelve el valor de lambda usado para penalizar el riesgo
     */
    double getLambda() const;

    /**
     * Devuelve el límite superior de inversión por empresa
     */
    double getHi() const;

    /**
     * Devuelve el número de días leídos
     */
    size_t getT() const;

    /**
     * Devuelve el límite inferior de inversión por empresa activa
     */
    double getLo() const;

    /**
     * Devuelve la matriz de retornos diarios
     */
    const vector<vector<double>>& getMu() const;
};
