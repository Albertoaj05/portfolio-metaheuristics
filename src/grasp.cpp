#include "../inc/grasp.h"
#include "../inc/portfolio.h"
#include "../common/random.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

using namespace std;

// Constructor con parametros por defecto del experimento extra
GRASP::GRASP(int iteraciones, int evaluaciones_bl, int tam_lrc) {
    this->iteraciones = iteraciones;
    this->evaluaciones_bl = evaluaciones_bl;
    this->tam_lrc = tam_lrc;
}

// Permite cambiar los parametros sin recompilar
void GRASP::setParametros(int iteraciones, int evaluaciones_bl, int tam_lrc) {
    this->iteraciones = iteraciones;
    this->evaluaciones_bl = evaluaciones_bl;
    this->tam_lrc = tam_lrc;
}

// Construye una solucion semi-voraz con lista restringida de candidatos
tSolution<double> GRASP::construirSolucion(ProblemDouble& problema) {

    // Convertimos el problema general al problema Portfolio
    Portfolio& portfolio = dynamic_cast<Portfolio&>(problema);

    // Obtenemos las estructuras que necesitamos del problema
    const vector<double>& beneficio = portfolio.getBeneficio();
    const vector<vector<double>>& cov = portfolio.getCov();

    double lambda = portfolio.getLambda(); // penalizacion del riesgo
    double hi = portfolio.getHi();         // maximo peso permitido por empresa
    int n = portfolio.getSolutionSize();   // numero de empresas

    // Vector donde guardamos, para cada empresa, su heuristica y su indice
    vector<pair<double, int>> heuristicas;

    // Calculamos la misma heuristica que en Greedy
    for (int i = 0; i < n; i++) {
        double riesgo_i = 0.0;

        for (int j = 0; j < n; j++) {
            riesgo_i += cov[i][j];
        }

        riesgo_i /= n;

        double heur = beneficio[i] - lambda * riesgo_i;
        heuristicas.push_back({heur, i});
    }

    // Ordenamos las empresas de mayor a menor heuristica
    sort(heuristicas.begin(), heuristicas.end(), greater<pair<double,int>>());

    // Construimos la solucion semi-voraz
    tSolution<double> solucion(n, 0.0);
    double suma = 0.0;

    while (suma < 1.0 && !heuristicas.empty()) {

        // Lista restringida: solo se puede elegir entre los tam_lrc mejores
        int limite = min(tam_lrc, (int)heuristicas.size());

        // En vez de coger siempre el mejor, elegimos uno aleatorio de la LRC
        int elegido = Random::get(0, limite - 1);

        int empresa = heuristicas[elegido].second;
        double valor = min(hi, 1.0 - suma);

        solucion[empresa] = valor;
        suma += valor;

        // Eliminamos la empresa elegida para no volver a seleccionarla
        heuristicas.erase(heuristicas.begin() + elegido);
    }

    return solucion;
}

// Ejecuta GRASP: construccion semi-voraz + BL
ResultMHDouble GRASP::optimize(ProblemDouble& problema, int max_evaluaciones) {

    // Comprobamos que el numero de evaluaciones sea valido
    assert(max_evaluaciones > 0);

    tSolution<double> mejor_solucion;
    double mejor_fitness = -1e18;
    int evaluaciones = 0;

    // Mismo esquema que BMB, pero con soluciones iniciales semi-voraces
    for (int i = 0; i < iteraciones; i++) {

        // Construimos una solucion inicial semi-voraz
        tSolution<double> solucion = construirSolucion(problema);

        // Aplicamos BL desde esa solucion, con evaluaciones_bl como maximo
        ResultMHDouble resultado = bl.optimizeFromSolution(problema, solucion, evaluaciones_bl);

        // Acumulamos las evaluaciones usadas por esta BL
        evaluaciones += resultado.evaluations;

        // Guardamos la mejor solucion encontrada entre todas las construcciones
        if (resultado.fitness > mejor_fitness) {
            mejor_fitness = resultado.fitness;
            mejor_solucion = resultado.solution;
        }
    }

    return ResultMHDouble(mejor_solucion, mejor_fitness, evaluaciones);
}