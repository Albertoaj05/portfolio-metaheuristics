#pragma once

#include <vector>
#include <string>
#include <mh.h>

using namespace std;

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementación de Algoritmos Genéticos para el problema de Portfolio
 *  - AGG: modelo generacional con elitismo
 *  - AGE: modelo estacionario
 *  - Cruces disponibles: aritmético aleatorio, BLX-alpha y uniforme
 *
 * @see MH
 * @see Problem
 */
class GeneticAlgorithm : public MHDouble {
private:
    /**
     * Representa un cromosoma de la población:
     *  - sol: vector de pesos
     *  - fitness: calidad de la solución
     */
    struct Individuo {
        tSolution<double> sol;
        double fitness;
    };

    int tamaño_poblacion;   // tamaño de la población
    int evaluaciones_max;   // número máximo de evaluaciones
    int k_torneo;           // tamaño del torneo

    double prob_cruce_AGG;  // probabilidad de cruce en AGG
    double prob_mutacion;   // probabilidad de mutación
    double alpha_BLX;       // parámetro alpha del cruce BLX

    string tipo_modelo;     // "AGG" o "AGE"
    string tipo_cruce;      // "ARIT", "BLX" o "UNIF"

    /**
     * Genera la población inicial aleatoria y evalúa cada individuo
     */
    vector<Individuo> inicializarPoblacion(ProblemDouble& problem, int& evaluaciones);

    /**
     * Selección por torneo de tamaño k_torneo
     */
    int seleccionarPorTorneo(const vector<Individuo>& poblacion);

    /**
     * Operador de cruce aritmético aleatorio
     */
    void cruceAritmetico(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2);

    /**
     * Operador de cruce BLX-alpha
     */
    void cruceBLX(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2, double alpha);

    /**
     * Operador de cruce uniforme
     */
    void cruceUniforme(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2);

    /**
     * Operador de mutación por intercambio de peso entre dos genes
     */
    void mutar(tSolution<double>& sol, ProblemDouble& problem);

    /**
     * Devuelve el índice del mejor individuo de la población
     */
    int mejorIndividuo(const vector<Individuo>& poblacion) const;

    /**
     * Devuelve el índice del peor individuo de la población
     */
    int peorIndividuo(const vector<Individuo>& poblacion) const;

    /**
     * Devuelve el índice del segundo peor individuo
     */
    int segundoPeorIndividuo(const vector<Individuo>& poblacion, int peor) const;

    /**
     * Ejecuta el algoritmo genético generacional con elitismo
     */
    ResultMHDouble ejecutarAGG(ProblemDouble& problem);

    /**
     * Ejecuta el algoritmo genético estacionario
     */
    ResultMHDouble ejecutarAGE(ProblemDouble& problem);

public:

    /**
     * Constructor:
     *  - modelo: "AGG" o "AGE"
     *  - cruce: "ARIT", "BLX" o "UNIF"
     */
    GeneticAlgorithm(const string& modelo, const string& cruce);

    /**
     * Destructor
     */
    virtual ~GeneticAlgorithm() {}

    /**
     * Ejecuta el algoritmo configurado sobre el problema
     */
    virtual ResultMH<double> optimize(Problem<double>& problem, int maxevals) override;

    /**
     * Modifica el parámetro alpha del cruce BLX-alpha
     */
    void setAlphaBLX(double a) { alpha_BLX = a; }

    /**
     * Modifica el tamaño del torneo de selección
     */
    void setKTorneo(int k) { k_torneo = k; }
};
