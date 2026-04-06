#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>
#include <map>

#include "portfolio.h"
#include "randomsearch.h"
#include "greedy.h"
#include "localsearch.h"
#include "random.hpp"

using namespace std;
using namespace std::chrono;

struct ConfiguracionDataset {
    string nombre;
    string fichero;
    double lo;
    double hi;
    string fichero_boxplot;
};

struct DatosPeriodo {
    vector<vector<double>> mu;
};

struct MetricasPeriodo {
    vector<double> beneficio_interno;
    vector<vector<double>> cov;
};

struct ResultadoEjecucion {
    double fitness_2015_2024 = 0.0;
    double fitness_2025 = 0.0;
    double beneficio_2025 = 0.0;
    double tiempo_seg = 0.0;
    int evaluaciones = 0;
};

struct ResultadoMedio {
    double fitness_2015_2024 = 0.0;
    double fitness_2025 = 0.0;
    double beneficio_2025 = 0.0;
    double tiempo_seg = 0.0;
    double evaluaciones = 0.0;
};

struct ConfiguracionExperimento {
    double lambda = 500.0;
    int num_ejecuciones = 50;
    int eval_random = 10000;
    int eval_bl = 10000;
    int eval_greedy = 1;
    int semilla_inicial = 42;
};

// Quita espacios al principio y al final
string trim(const string& s) {
    size_t ini = s.find_first_not_of(" \t\r\n");
    if (ini == string::npos) return "";
    size_t fin = s.find_last_not_of(" \t\r\n");
    return s.substr(ini, fin - ini + 1);
}

// Lee fichero de configuración opcional
ConfiguracionExperimento leerConfiguracion(const string& fichero_config) {
    ConfiguracionExperimento cfg;
    ifstream file(fichero_config);

    if (!file.is_open()) {
        throw runtime_error("No se pudo abrir el fichero de configuración: " + fichero_config);
    }

    string line;
    while (getline(file, line)) {
        line = trim(line);

        if (line.empty()) continue;
        if (line[0] == '#') continue;

        size_t pos = line.find('=');
        if (pos == string::npos) continue;

        string clave = trim(line.substr(0, pos));
        string valor = trim(line.substr(pos + 1));

        if (clave == "lambda") cfg.lambda = stod(valor);
        else if (clave == "num_ejecuciones") cfg.num_ejecuciones = stoi(valor);
        else if (clave == "eval_random") cfg.eval_random = stoi(valor);
        else if (clave == "eval_bl") cfg.eval_bl = stoi(valor);
        else if (clave == "eval_greedy") cfg.eval_greedy = stoi(valor);
        else if (clave == "semilla_inicial") cfg.semilla_inicial = stoi(valor);
    }

    return cfg;
}

// Lee del CSV solo las filas entre dos fechas
DatosPeriodo cargarPeriodoCSV(const string& fichero, const string& fecha_inicio, const string& fecha_fin) {
    DatosPeriodo datos;
    ifstream file(fichero);

    if (!file.is_open()) {
        throw runtime_error("No se pudo abrir el fichero: " + fichero);
    }

    string line;
    getline(file, line); // saltamos cabecera

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string celda, fecha;
        vector<double> fila;

        getline(ss, fecha, ',');

        if (fecha < fecha_inicio || fecha > fecha_fin) {
            continue;
        }

        while (getline(ss, celda, ',')) {
            fila.push_back(stod(celda));
        }

        if (!fila.empty()) {
            datos.mu.push_back(fila);
        }
    }

    return datos;
}

// Calcula beneficio interno y covarianza para un periodo
MetricasPeriodo calcularMetricasPeriodo(const DatosPeriodo& datos) {
    MetricasPeriodo metricas;

    if (datos.mu.empty()) {
        return metricas;
    }

    const int T = (int)datos.mu.size();
    const int n = (int)datos.mu[0].size();

    metricas.beneficio_interno.assign(n, 0.0);
    metricas.cov.assign(n, vector<double>(n, 0.0));

    vector<double> medias(n, 0.0);

    for (int j = 0; j < n; j++) {
        double suma_mu = 0.0;
        double suma_log = 0.0;

        for (int t = 0; t < T; t++) {
            suma_mu += datos.mu[t][j];
            suma_log += log(1.0 + datos.mu[t][j]);
        }

        medias[j] = suma_mu / T;
        metricas.beneficio_interno[j] = suma_log;
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double suma = 0.0;

            for (int t = 0; t < T; t++) {
                suma += (datos.mu[t][i] - medias[i]) * (datos.mu[t][j] - medias[j]);
            }

            metricas.cov[i][j] = suma / T;
        }
    }

    return metricas;
}

// Evalúa una solución usando las métricas de un periodo concreto
pair<double, double> evaluarSolucionEnPeriodo(
    const tSolution<double>& w,
    const MetricasPeriodo& metricas,
    double lambda
) {
    if (metricas.beneficio_interno.empty() || metricas.cov.empty()) {
        return {0.0, 0.0};
    }

    double beneficio = 0.0;
    double riesgo = 0.0;

    for (size_t i = 0; i < w.size(); i++) {
        beneficio += w[i] * metricas.beneficio_interno[i];
    }

    for (size_t i = 0; i < w.size(); i++) {
        for (size_t j = 0; j < w.size(); j++) {
            riesgo += w[i] * metricas.cov[i][j] * w[j];
        }
    }

    double fitness = beneficio - lambda * riesgo;
    return {fitness, beneficio};
}

// Calcula la media de todas las ejecuciones
ResultadoMedio calcularMedia(const vector<ResultadoEjecucion>& runs) {
    ResultadoMedio medias;

    if (runs.empty()) return medias;

    for (size_t i = 0; i < runs.size(); i++) {
        medias.fitness_2015_2024 += runs[i].fitness_2015_2024;
        medias.fitness_2025 += runs[i].fitness_2025;
        medias.beneficio_2025 += runs[i].beneficio_2025;
        medias.tiempo_seg += runs[i].tiempo_seg;
        medias.evaluaciones += runs[i].evaluaciones;
    }

    const double total = (double)runs.size();
    medias.fitness_2015_2024 /= total;
    medias.fitness_2025 /= total;
    medias.beneficio_2025 /= total;
    medias.tiempo_seg /= total;
    medias.evaluaciones /= total;

    return medias;
}

// Imprime por pantalla la tabla resumen de un dataset
void imprimirTablaDataset(const string& nombre_dataset,
                          const ResultadoMedio& greedy,
                          const ResultadoMedio& random,
                          const ResultadoMedio& bl) {
    cout << "\n=============================================================\n";
    cout << "Resultados de " << nombre_dataset << " (medias)\n";
    cout << "=============================================================\n";

    cout << left
         << setw(12) << "Algoritmo"
         << setw(15) << "2015-2024"
         << setw(15) << "2025 Fit."
         << setw(15) << "2025 Benef."
         << setw(15) << "Evaluaciones"
         << setw(12) << "Tiempo(s)"
         << "\n";

    cout << fixed << setprecision(3);

    auto imprimirFila = [](const string& nombre, const ResultadoMedio& m) {
        cout << left
             << setw(12) << nombre
             << setw(15) << m.fitness_2015_2024
             << setw(15) << m.fitness_2025
             << setw(15) << m.beneficio_2025
             << setw(15) << m.evaluaciones
             << setw(12) << m.tiempo_seg
             << "\n";
    };

    imprimirFila("Greedy", greedy);
    imprimirFila("Random", random);
    imprimirFila("BL", bl);
}

int main(int argc, char* argv[]) {
    try {
        // Si no se pasa ningún fichero, usamos exactamente los valores por defecto de la práctica
        ConfiguracionExperimento cfg;

        // Si se pasa un fichero de configuración, lo leemos
        if (argc >= 2) {
            cfg = leerConfiguracion(argv[1]);
        }

        vector<ConfiguracionDataset> datasets = {
            {"Ibex 35", "datos_portfolio_2526/ibex_35.csv", 0.005, 0.08, "resultados/boxplot_ibex_35.csv"},
            {"S&P 100", "datos_portfolio_2526/syp_100.csv", 0.005, 0.05, "resultados/boxplot_syp_100.csv"},
            {"S&P 500", "datos_portfolio_2526/syp_500.csv", 0.005, 0.02, "resultados/boxplot_syp_500.csv"}
        };

        // Generamos las semillas a partir de la semilla inicial
        vector<int> semillas;
        for (int i = 0; i < cfg.num_ejecuciones; i++) {
            semillas.push_back(cfg.semilla_inicial + i);
        }

        // Guardamos las semillas usadas
        ofstream semillas_csv("resultados/semillas_utilizadas.csv");
        if (!semillas_csv.is_open()) {
            throw runtime_error("No se pudo crear resultados/semillas_utilizadas.csv");
        }

        semillas_csv << "repeticion,semilla\n";
        for (int i = 0; i < cfg.num_ejecuciones; i++) {
            semillas_csv << (i + 1) << "," << semillas[i] << "\n";
        }
        semillas_csv.close();

        // Guardamos también la configuración usada
        ofstream config_csv("resultados/configuracion_usada.txt");
        if (!config_csv.is_open()) {
            throw runtime_error("No se pudo crear resultados/configuracion_usada.txt");
        }

        config_csv << "lambda=" << cfg.lambda << "\n";
        config_csv << "num_ejecuciones=" << cfg.num_ejecuciones << "\n";
        config_csv << "eval_random=" << cfg.eval_random << "\n";
        config_csv << "eval_bl=" << cfg.eval_bl << "\n";
        config_csv << "eval_greedy=" << cfg.eval_greedy << "\n";
        config_csv << "semilla_inicial=" << cfg.semilla_inicial << "\n";
        config_csv.close();

        ofstream resumen_csv("resultados/resultados_medias.csv");
        if (!resumen_csv.is_open()) {
            throw runtime_error("No se pudo crear resultados/resultados_medias.csv");
        }

        resumen_csv << "dataset,algoritmo,fitness_2015_2024,fitness_2025,beneficio_2025,evaluaciones,tiempo_seg\n";
        resumen_csv << fixed << setprecision(6);

        RandomSearch<double> randomAlg;
        GreedySearch greedyAlg;
        LocalSearch blAlg;

        for (size_t d = 0; d < datasets.size(); d++) {
            ConfiguracionDataset ds = datasets[d];

            cout << "\n##################################################\n";
            cout << "DATASET: " << ds.nombre << "\n";
            cout << "##################################################\n";

            // Conjunto de entrenamiento: 2015-2024
            Portfolio portfolio_train(ds.fichero, ds.lo, ds.hi, cfg.lambda);

            // Conjunto de evaluación: 2025
            DatosPeriodo datos_2025 = cargarPeriodoCSV(ds.fichero, "2025-01-01", "2025-12-31");
            MetricasPeriodo metricas_2025 = calcularMetricasPeriodo(datos_2025);

            vector<ResultadoEjecucion> runs_greedy;
            vector<ResultadoEjecucion> runs_random;
            vector<ResultadoEjecucion> runs_bl;

            ofstream boxplot_csv(ds.fichero_boxplot);
            if (!boxplot_csv.is_open()) {
                throw runtime_error("No se pudo crear " + ds.fichero_boxplot);
            }

            boxplot_csv << "algoritmo,fitness_2015_2024\n";
            boxplot_csv << fixed << setprecision(6);

            for (int rep = 0; rep < cfg.num_ejecuciones; rep++) {
                int semilla = semillas[rep];

                // RANDOM
                Random::seed(semilla);
                auto inicio_r = high_resolution_clock::now();
                auto res_r = randomAlg.optimize(portfolio_train, cfg.eval_random);
                auto fin_r = high_resolution_clock::now();

                ResultadoEjecucion r1;
                r1.tiempo_seg = duration<double>(fin_r - inicio_r).count();
                r1.fitness_2015_2024 = res_r.fitness;
                r1.evaluaciones = res_r.evaluations;

                auto eval_r = evaluarSolucionEnPeriodo(res_r.solution, metricas_2025, cfg.lambda);
                r1.fitness_2025 = eval_r.first;
                r1.beneficio_2025 = eval_r.second;

                runs_random.push_back(r1);
                boxplot_csv << "Random," << r1.fitness_2015_2024 << "\n";

                // GREEDY
                Random::seed(semilla);
                auto inicio_g = high_resolution_clock::now();
                auto res_g = greedyAlg.optimize(portfolio_train, cfg.eval_greedy);
                auto fin_g = high_resolution_clock::now();

                ResultadoEjecucion r2;
                r2.tiempo_seg = duration<double>(fin_g - inicio_g).count();
                r2.fitness_2015_2024 = res_g.fitness;
                r2.evaluaciones = res_g.evaluations;

                auto eval_g = evaluarSolucionEnPeriodo(res_g.solution, metricas_2025, cfg.lambda);
                r2.fitness_2025 = eval_g.first;
                r2.beneficio_2025 = eval_g.second;

                runs_greedy.push_back(r2);
                boxplot_csv << "Greedy," << r2.fitness_2015_2024 << "\n";

                // BL
                Random::seed(semilla);
                auto inicio_b = high_resolution_clock::now();
                auto res_b = blAlg.optimize(portfolio_train, cfg.eval_bl);
                auto fin_b = high_resolution_clock::now();

                ResultadoEjecucion r3;
                r3.tiempo_seg = duration<double>(fin_b - inicio_b).count();
                r3.fitness_2015_2024 = res_b.fitness;
                r3.evaluaciones = res_b.evaluations;

                auto eval_b = evaluarSolucionEnPeriodo(res_b.solution, metricas_2025, cfg.lambda);
                r3.fitness_2025 = eval_b.first;
                r3.beneficio_2025 = eval_b.second;

                runs_bl.push_back(r3);
                boxplot_csv << "BL," << r3.fitness_2015_2024 << "\n";
            }

            boxplot_csv.close();

            ResultadoMedio media_greedy = calcularMedia(runs_greedy);
            ResultadoMedio media_random = calcularMedia(runs_random);
            ResultadoMedio media_bl = calcularMedia(runs_bl);

            imprimirTablaDataset(ds.nombre, media_greedy, media_random, media_bl);

            resumen_csv << ds.nombre << ",Greedy,"
                        << media_greedy.fitness_2015_2024 << ","
                        << media_greedy.fitness_2025 << ","
                        << media_greedy.beneficio_2025 << ","
                        << media_greedy.evaluaciones << ","
                        << media_greedy.tiempo_seg << "\n";

            resumen_csv << ds.nombre << ",Random,"
                        << media_random.fitness_2015_2024 << ","
                        << media_random.fitness_2025 << ","
                        << media_random.beneficio_2025 << ","
                        << media_random.evaluaciones << ","
                        << media_random.tiempo_seg << "\n";

            resumen_csv << ds.nombre << ",BL,"
                        << media_bl.fitness_2015_2024 << ","
                        << media_bl.fitness_2025 << ","
                        << media_bl.beneficio_2025 << ","
                        << media_bl.evaluaciones << ","
                        << media_bl.tiempo_seg << "\n";
        }

        resumen_csv.close();

        cout << "\nFicheros generados:\n";
        cout << "- resultados/resultados_medias.csv\n";
        cout << "- resultados/semillas_utilizadas.csv\n";
        cout << "- resultados/configuracion_usada.txt\n";
        cout << "- resultados/boxplot_ibex_35.csv\n";
        cout << "- resultados/boxplot_syp_100.csv\n";
        cout << "- resultados/boxplot_syp_500.csv\n";
        cout << "\nSi quieres cambiar parametros, ejecuta por ejemplo:\n";
        cout << "./main config.txt\n";

        return 0;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}