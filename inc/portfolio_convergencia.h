#pragma once

#include "portfolio.h"

#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace std;

class PortfolioConvergencia : public Portfolio {
private:
    ofstream& fichero_salida;
    string nombre_algoritmo;
    int contador_evaluaciones;
    double mejor_fitness_encontrado;
    vector<pair<int, double>> curva_convergencia;

public:
    PortfolioConvergencia(const string& filename, double lo, double hi, double lambda, ofstream& csv, const string& algoritmo) 
    : Portfolio(filename, lo, hi, lambda),fichero_salida(csv), nombre_algoritmo(algoritmo), contador_evaluaciones(0),  mejor_fitness_encontrado(-numeric_limits<double>::infinity()) {}

    ~PortfolioConvergencia() {
        // Al terminar la ejecución, guardamos todos los puntos de la curva.
        for (const auto& punto : curva_convergencia) {
            fichero_salida << nombre_algoritmo << "," << punto.first << "," << punto.second << "\n";
        }
    }

    tFitness fitness(const tSolution<double>& solution) override {
        // Calculamos el fitness normal del portfolio.
        double fit = Portfolio::fitness(solution);
        contador_evaluaciones++;

        // Guardamos siempre el mejor valor visto hasta esta evaluación.
        if (fit > mejor_fitness_encontrado) {
            mejor_fitness_encontrado = fit;
        }

        curva_convergencia.push_back({contador_evaluaciones, mejor_fitness_encontrado});

        return fit;
    }
};
