#pragma once
#include <mh.h>
#include <vector>
#include <utility>

using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

/**
 * Implementación de la Búsqueda Local (BL)
 *  - Parte de una solución inicial aleatoria
 *  - Explora vecinos moviendo peso entre dos posiciones
 *  - Acepta la primera mejora encontrada (first improvement)
 *
 * @see MH
 * @see Problem
 */
class LocalSearch : public MHDouble {

public:
  LocalSearch() : MH<double>() {}
  virtual ~LocalSearch() {}

  /**
   * Aplica búsqueda local primero mejor:
   *  - Explora el entorno definido por pares (i, j) con i != j
   *  - Genera vecinos moviendo un porcentaje del peso entre posiciones
   *  - Se detiene al encontrar la primera mejora o al agotar evaluaciones
   *
   * @param problem Problema a optimizar
   * @param maxevals Número máximo de evaluaciones
   * @return Mejor solución encontrada y su fitness
   */
  virtual ResultMH<double> optimize(Problem<double> &problem, int maxevals) override;

private:
  /**
   * Genera todos los pares (i, j) con i != j que forman el entorno
   */
  std::vector<std::pair<int,int>> generarEntorno(int n);

  /**
   * Genera un vecino moviendo un porcentaje del peso de i hacia j
   * y comprueba si cumple los límites
   */
  bool generarVecino(const tSolution<double>& actual, tSolution<double>& vecino,
                     int i, int j, double ratio, double lo, double hi);
};