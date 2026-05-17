#pragma once
#include <mh.h>
using namespace std;

/**
 * Implementation of the Random Search metaheuristic
 *  - Randomly generates solutions and selects the best one
 *
 * @see MH
 * @see Problem
 */
template <typename tDomain> class RandomSearch : public MH<tDomain> {

private:
  ResultMH<tDomain> optimizar(Problem<tDomain>& problem, int num_evaluaciones) {
    // Comprobamos que el número de evaluciones es válido
    assert(num_evaluaciones > 0);
    tSolution<tDomain> mejorSolucion;
    tFitness mejor_fitness = -1e18;

    for (int i = 0; i < num_evaluaciones; i++) {
      // Generamos una solución aleatoria
      tSolution<tDomain> solucion = problem.createSolution();

      // Evaluamos el fitness de la solución
      tFitness fitness = problem.fitness(solucion);

      // Si es mejor que la mejor encontrada hasta ahora, la guardamos
      if (fitness > mejor_fitness) {
        mejorSolucion = solucion;
        mejor_fitness = fitness;
      }

    }

    return ResultMH<tDomain>(mejorSolucion, mejor_fitness, num_evaluaciones);
  }

public:
  RandomSearch() : MH<tDomain>() {}
  virtual ~RandomSearch() {}
  // Implement the MH interface methods
  /**
   * Create random solutions until maxevals has been achieved, and returns the
   * best one.
   *
   * @param problem The problem to be optimized
   * @param maxevals Maximum number of evaluations allowed
   * @return A pair containing the best solution found and its fitness
   */
  ResultMH<tDomain> optimize(Problem<tDomain> &problem, int num_evaluaciones) override {
    return optimizar(problem, num_evaluaciones);
  }
};
