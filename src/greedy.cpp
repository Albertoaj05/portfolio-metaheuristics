#include <cassert>
#include <algorithm>
#include <greedy.h>
#include <iostream>
#include "../inc/portfolio.h"

using namespace std;

// Algoritmo Greedy que construye una solución asignando más peso a las empresas con mejor relación beneficio-riesgo hasta completar la inversión total.
ResultMHDouble GreedySearch::optimize(ProblemDouble &problema, int num_evaluaciones) {

  // Comprobamos que el número de evaluaciones sea válido
  assert(num_evaluaciones > 0); 

  // Convertimos el problema general al problema Portfolio
  Portfolio& portfolio = dynamic_cast<Portfolio &>(problema);

  // Obtenemos las estructuras que necesitamos del problema
  const vector<double> &beneficio = portfolio.getBeneficio();
  const vector<vector<double>> &cov = portfolio.getCov();

  double lambda = portfolio.getLambda(); // penalización del riesgo
  double hi = portfolio.getHi();         // máximo peso permitido por empresa
  int n = portfolio.getSolutionSize();   // número de empresas

  // Vector donde guardamos, para cada empresa, su heurística y su índice
  vector<pair<double, int>> heuristicas;

  // Calculamos la heurística de cada empresa
  for (int i = 0; i < n; i++) {
    double riesgo_i = 0.0;
    for (int j = 0; j < n; j++) {
      riesgo_i += cov[i][j];
    }

    riesgo_i /= n;
    double heur = beneficio[i] - lambda * riesgo_i;
    heuristicas.push_back({heur, i});
  }

  // Ordenamos las empresas de mayor a menor heurística
  sort(heuristicas.begin(), heuristicas.end(), greater<pair<double,int>>());

    // Construimos la solución greedy
    tSolution<double> solucion(n, 0.0);
    double suma = 0.0;
    int pos = 0;

    while (suma < 1.0 && pos < n) {
        int empresa = heuristicas[pos].second;
        double valor = min(hi, 1.0 - suma);

        solucion[empresa] = valor;
        suma += valor;
        pos++;
    }

    portfolio.fix(solucion);

    // Calculamos el fitness de la solución final
    double fitness = portfolio.fitness(solucion);

    return ResultMH<double>(solucion, fitness, 1);
}