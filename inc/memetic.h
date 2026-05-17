#pragma once

#include <vector>
#include <string>
#include <mh.h>

using namespace std;

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementación de Algoritmos Meméticos para el problema de Portfolio
 *  - Usa como base un AGG
 *  - Aplica una Búsqueda Local suave cada 10 generaciones
 *  - Variantes: AM-All, AM-Rand, AM-Best y AM-Best1
 *
 * @see MH
 * @see Problem
 */
class MemeticAlgorithm : public MHDouble {
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

    double prob_cruce;      // probabilidad de cruce en el AGG base
    double prob_mutacion;   // probabilidad de mutación
    double alpha_BLX;       // parámetro alpha del cruce BLX

    string tipo_am;         // "ALL", "RAND", "BEST" o "BEST1"
    string tipo_cruce;      // "ARIT" o "BLX"

    /**
     * Genera la población inicial aleatoria y evalúa sus individuos
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
     * Genera un vecino moviendo un porcentaje del peso de i hacia j
     * y comprueba si cumple los límites
     */
    bool generarVecino(const tSolution<double>& actual, tSolution<double>& vecino, int i, int j, double ratio, double lo, double hi);
    
    /**
     * Genera todos los pares (i, j) con i != j que forman el entorno
     */
    vector<pair<int,int>> generarEntorno(size_t n);

    /**
     * Aplica una Búsqueda Local suave con ratio 0.15
     */
    void busquedaLocalSuave(tSolution<double>& sol, double& fitness_sol, ProblemDouble& problem, int& evaluaciones);

    /**
     * Decide a qué cromosomas aplicar la BL según la variante del AM
     */
    void aplicarBL(vector<Individuo>& poblacion, ProblemDouble& problem, int& evaluaciones);

    /**
     * Ejecuta el algoritmo memético
     */
    ResultMH<double> ejecutarAM(Problem<double>& problem);

public:

    /**
     * Constructor:
     *  - tipo: "ALL", "RAND", "BEST" o "BEST1"
     *  - cruce: "ARIT" o "BLX"
     */
    MemeticAlgorithm(const string& tipo, const string& cruce);

    /**
     * Destructor
     */
    virtual ~MemeticAlgorithm() {}

    /**
     * Ejecuta el algoritmo configurado sobre el problema
     */
    virtual ResultMH<double> optimize(Problem<double>& problem, int maxevals) override;

};
