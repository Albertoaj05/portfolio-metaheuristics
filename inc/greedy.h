#pragma once
#include <mh.h>

using namespace std;

/**
 * Implementación de la metaheurística Greedy (voraz)
 *  - Construye una solución eligiendo primero las mejores empresas
 *  - Se basa en una heurística que combina beneficio y riesgo
 *
 * @see MH
 * @see Problem
 */
using MHDouble = MH<double>;
using ProblemDouble = Problem<double>;
using ResultMHDouble = ResultMH<double>;

class GreedySearch : public MHDouble {

public:
  GreedySearch() : MH() {}
  virtual ~GreedySearch() {}

  /**
   * Construye una solución de forma voraz:
   *  - Ordena las empresas según una heurística
   *  - Va asignando peso a las mejores hasta completar la inversión
   *
   * @param problem Problema a optimizar
   * @param maxevals Número máximo de evaluaciones (no relevante aquí, ya que Greedy evalúa una vez)
   * @return Mejor solución encontrada y su fitness
   */
  virtual ResultMH<double> optimize(Problem<double> &problem, int maxevals) override;
};