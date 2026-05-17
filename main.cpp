#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>
#include <map>
#include <cmath>

#include "portfolio.h"
#include "portfolio_convergencia.h"
#include "randomsearch.h"
#include "greedy.h"
#include "localsearch.h"
#include "genetic.h"
#include "memetic.h"
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

struct ResultadoExtra {
    string dataset;
    string algoritmo;
    ResultadoMedio media;
};

struct ConfiguracionExperimento {
    double lambda = 500.0;
    int num_ejecuciones = 50;
    int eval_random = 10000;
    int eval_bl = 10000;
    int eval_greedy = 1;
    int eval_ag = 10000;
    int eval_am = 10000;
    int semilla_inicial = 42;
};

// Limpia espacios al leer pares clave=valor del fichero de configuración.
string trim(const string& s) {
    size_t ini = s.find_first_not_of(" \t\r\n");
    if (ini == string::npos) return "";
    size_t fin = s.find_last_not_of(" \t\r\n");
    return s.substr(ini, fin - ini + 1);
}

// Lee parámetros opcionales desde un fichero de configuración.
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
        else if (clave == "eval_ag") cfg.eval_ag = stoi(valor);
        else if (clave == "semilla_inicial") cfg.semilla_inicial = stoi(valor);
        else if (clave == "eval_am") cfg.eval_am = stoi(valor);
    }

    return cfg;
}

// Carga solo las filas de un intervalo de fechas, usado para evaluar 2025.
DatosPeriodo cargarPeriodoCSV(const string& fichero, const string& fecha_inicio, const string& fecha_fin) {
    DatosPeriodo datos;
    ifstream file(fichero);

    if (!file.is_open()) {
        throw runtime_error("No se pudo abrir el fichero: " + fichero);
    }

    string line;
    getline(file, line);

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

// Calcula beneficio acumulado y covarianzas para el periodo de test.
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

// Evalúa una solución ya obtenida sobre otro periodo de datos.
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

    if (riesgo < 0.0 && fabs(riesgo) < 1e-12) {
        riesgo = 0.0;
    }

    riesgo = sqrt(riesgo);

    double fitness = beneficio - lambda * riesgo;
    return {fitness, beneficio};
}

// Promedia las repeticiones de un mismo algoritmo.
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

    double total = (double)runs.size();

    medias.fitness_2015_2024 /= total;
    medias.fitness_2025 /= total;
    medias.beneficio_2025 /= total;
    medias.tiempo_seg /= total;
    medias.evaluaciones /= total;

    return medias;
}

// Imprime una fila de la tabla principal.
void imprimirFila(const string& nombre, const ResultadoMedio& m) {
    cout << left
         << setw(12) << nombre
         << setw(15) << m.fitness_2015_2024
         << setw(15) << m.fitness_2025
         << setw(15) << m.beneficio_2025
         << setw(15) << m.evaluaciones
         << setw(12) << m.tiempo_seg
         << "\n";
}

int main(int argc, char* argv[]) {
    try {
        ConfiguracionExperimento cfg;

        if (argc >= 2) {
            cfg = leerConfiguracion(argv[1]);
        }

        // Datasets y límites de inversión usados en los experimentos.
        vector<ConfiguracionDataset> datasets = {
            {"Ibex 35", "datos_portfolio_2526/ibex_35.csv", 0.005, 0.08, "resultados/boxplot_ibex_35.csv"},
            {"S&P 100", "datos_portfolio_2526/syp_100.csv", 0.005, 0.05, "resultados/boxplot_syp_100.csv"},
            {"S&P 500", "datos_portfolio_2526/syp_500.csv", 0.005, 0.02, "resultados/boxplot_syp_500.csv"}
        };

        // Semillas consecutivas para que las ejecuciones sean reproducibles.
        vector<int> semillas;
        for (int i = 0; i < cfg.num_ejecuciones; i++) {
            semillas.push_back(cfg.semilla_inicial + i);
        }

        ofstream semillas_csv("resultados/semillas_utilizadas.csv");
        if (!semillas_csv.is_open()) {
            throw runtime_error("No se pudo crear resultados/semillas_utilizadas.csv");
        }

        semillas_csv << "repeticion,semilla\n";
        for (int i = 0; i < cfg.num_ejecuciones; i++) {
            semillas_csv << (i + 1) << "," << semillas[i] << "\n";
        }
        semillas_csv.close();

        ofstream config_csv("resultados/configuracion_usada.txt");
        if (!config_csv.is_open()) {
            throw runtime_error("No se pudo crear resultados/configuracion_usada.txt");
        }

        config_csv << "lambda=" << cfg.lambda << "\n";
        config_csv << "num_ejecuciones=" << cfg.num_ejecuciones << "\n";
        config_csv << "eval_random=" << cfg.eval_random << "\n";
        config_csv << "eval_bl=" << cfg.eval_bl << "\n";
        config_csv << "eval_greedy=" << cfg.eval_greedy << "\n";
        config_csv << "eval_ag=" << cfg.eval_ag << "\n";
        config_csv << "eval_am=" << cfg.eval_am << "\n";
        config_csv << "semilla_inicial=" << cfg.semilla_inicial << "\n";
        config_csv.close();

        ofstream resumen_csv("resultados/resultados_medias.csv");
        if (!resumen_csv.is_open()) {
            throw runtime_error("No se pudo crear resultados/resultados_medias.csv");
        }

        resumen_csv << "dataset,algoritmo,fitness_2015_2024,fitness_2025,beneficio_2025,evaluaciones,tiempo_seg\n";
        resumen_csv << fixed << setprecision(6);

        // Algoritmos obligatorios y variantes extra que se muestran aparte.
        RandomSearch<double> randomAlg;
        GreedySearch greedyAlg;
        LocalSearch blAlg;

        GeneticAlgorithm aggArit("AGG", "ARIT");
        GeneticAlgorithm aggBlx("AGG", "BLX");
        GeneticAlgorithm ageArit("AGE", "ARIT");
        GeneticAlgorithm ageBlx("AGE", "BLX");
        GeneticAlgorithm aggUnif("AGG", "UNIF");
        GeneticAlgorithm ageUnif("AGE", "UNIF");
        MemeticAlgorithm amAll("ALL", "BLX");
        MemeticAlgorithm amRand("RAND", "BLX");
        MemeticAlgorithm amBest("BEST", "BLX");
        MemeticAlgorithm amBest1("BEST1", "BLX");

        vector<pair<string, GeneticAlgorithm*>> algoritmos_geneticos = {
            {"AGG-Arit", &aggArit},
            {"AGG-BLX", &aggBlx},
            {"AGE-Arit", &ageArit},
            {"AGE-BLX", &ageBlx}
        };

        vector<pair<string, GeneticAlgorithm*>> algoritmos_geneticos_extra = {
            {"AGG-Unif", &aggUnif},
            {"AGE-Unif", &ageUnif}
        };

        vector<pair<string, MemeticAlgorithm*>> algoritmos_memeticos = {
            {"AM-All", &amAll},
            {"AM-Rand", &amRand},
            {"AM-Best", &amBest}
        };

        vector<pair<string, MemeticAlgorithm*>> algoritmos_memeticos_extra = {
            {"AM-Best1", &amBest1} 
        };

        vector<ResultadoExtra> resultados_extra;

        // Ejecutamos todos los algoritmos sobre cada dataset.
        for (size_t d = 0; d < datasets.size(); d++) {
            ConfiguracionDataset ds = datasets[d];

            cout << "\n##################################################\n";
            cout << "DATASET: " << ds.nombre << "\n";
            cout << "##################################################\n";

            Portfolio portfolio_train(ds.fichero, ds.lo, ds.hi, cfg.lambda);

            // 2025 se calcula aparte para medir cómo generaliza cada solución.
            DatosPeriodo datos_2025 = cargarPeriodoCSV(ds.fichero, "2025-01-01", "2025-12-31");
            MetricasPeriodo metricas_2025 = calcularMetricasPeriodo(datos_2025);

            vector<ResultadoEjecucion> runs_greedy;
            vector<ResultadoEjecucion> runs_random;
            vector<ResultadoEjecucion> runs_bl;
            map<string, vector<ResultadoEjecucion>> runs_geneticos;
            map<string, vector<ResultadoEjecucion>> runs_memeticos;
            map<string, vector<ResultadoEjecucion>> runs_geneticos_extra;
            map<string, vector<ResultadoEjecucion>> runs_memeticos_extra;

            ofstream boxplot_csv(ds.fichero_boxplot);
            if (!boxplot_csv.is_open()) {
                throw runtime_error("No se pudo crear " + ds.fichero_boxplot);
            }

            boxplot_csv << "algoritmo,fitness_2015_2024\n";
            boxplot_csv << fixed << setprecision(6);

            // En cada repetición se usa la misma semilla para todos los algoritmos.
            for (int rep = 0; rep < cfg.num_ejecuciones; rep++) {
                int semilla = semillas[rep];

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

                for (auto& alg : algoritmos_geneticos) {
                    string nombre_alg = alg.first;
                    GeneticAlgorithm* algoritmo = alg.second;

                    Random::seed(semilla);
                    auto inicio = high_resolution_clock::now();
                    auto res = algoritmo->optimize(portfolio_train, cfg.eval_ag);
                    auto fin = high_resolution_clock::now();

                    ResultadoEjecucion r;
                    r.tiempo_seg = duration<double>(fin - inicio).count();
                    r.fitness_2015_2024 = res.fitness;
                    r.evaluaciones = res.evaluations;

                    auto eval = evaluarSolucionEnPeriodo(res.solution, metricas_2025, cfg.lambda);
                    r.fitness_2025 = eval.first;
                    r.beneficio_2025 = eval.second;

                    runs_geneticos[nombre_alg].push_back(r);
                    boxplot_csv << nombre_alg << "," << r.fitness_2015_2024 << "\n";
                }
                for (auto& alg : algoritmos_memeticos) {
                    string nombre_alg = alg.first;
                    MemeticAlgorithm* algoritmo = alg.second;

                    Random::seed(semilla);
                    auto inicio = high_resolution_clock::now();
                    auto res = algoritmo->optimize(portfolio_train, cfg.eval_am);
                    auto fin = high_resolution_clock::now();

                    ResultadoEjecucion r;
                    r.tiempo_seg = duration<double>(fin - inicio).count();
                    r.fitness_2015_2024 = res.fitness;
                    r.evaluaciones = res.evaluations;

                    auto eval = evaluarSolucionEnPeriodo(res.solution, metricas_2025, cfg.lambda);
                    r.fitness_2025 = eval.first;
                    r.beneficio_2025 = eval.second;

                    runs_memeticos[nombre_alg].push_back(r);
                    boxplot_csv << nombre_alg << "," << r.fitness_2015_2024 << "\n";
                }

                for (auto& alg : algoritmos_geneticos_extra) {
                    string nombre_alg = alg.first;
                    GeneticAlgorithm* algoritmo = alg.second;

                    Random::seed(semilla);
                    auto inicio = high_resolution_clock::now();
                    auto res = algoritmo->optimize(portfolio_train, cfg.eval_ag);
                    auto fin = high_resolution_clock::now();

                    ResultadoEjecucion r;
                    r.tiempo_seg = duration<double>(fin - inicio).count();
                    r.fitness_2015_2024 = res.fitness;
                    r.evaluaciones = res.evaluations;

                    auto eval = evaluarSolucionEnPeriodo(res.solution, metricas_2025, cfg.lambda);
                    r.fitness_2025 = eval.first;
                    r.beneficio_2025 = eval.second;

                    runs_geneticos_extra[nombre_alg].push_back(r);
                    boxplot_csv << nombre_alg << "," << r.fitness_2015_2024 << "\n";
                }

                for (auto& alg : algoritmos_memeticos_extra) {
                    string nombre_alg = alg.first;
                    MemeticAlgorithm* algoritmo = alg.second;

                    Random::seed(semilla);
                    auto inicio = high_resolution_clock::now();
                    auto res = algoritmo->optimize(portfolio_train, cfg.eval_am);
                    auto fin = high_resolution_clock::now();

                    ResultadoEjecucion r;
                    r.tiempo_seg = duration<double>(fin - inicio).count();
                    r.fitness_2015_2024 = res.fitness;
                    r.evaluaciones = res.evaluations;

                    auto eval = evaluarSolucionEnPeriodo(res.solution, metricas_2025, cfg.lambda);
                    r.fitness_2025 = eval.first;
                    r.beneficio_2025 = eval.second;

                    runs_memeticos_extra[nombre_alg].push_back(r);
                    boxplot_csv << nombre_alg << "," << r.fitness_2015_2024 << "\n";
                }
            }

            boxplot_csv.close();

            ResultadoMedio media_greedy = calcularMedia(runs_greedy);
            ResultadoMedio media_random = calcularMedia(runs_random);
            ResultadoMedio media_bl = calcularMedia(runs_bl);

            // Tabla principal con los algoritmos obligatorios.
            cout << "\n=============================================================\n";
            cout << "Resultados de " << ds.nombre << " (medias)\n";
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

            imprimirFila("Greedy", media_greedy);
            imprimirFila("Random", media_random);
            imprimirFila("BL", media_bl);

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

            for (auto& par : runs_geneticos) {
                string nombre_alg = par.first;
                ResultadoMedio media = calcularMedia(par.second);

                imprimirFila(nombre_alg, media);

                resumen_csv << ds.nombre << "," << nombre_alg << ","
                            << media.fitness_2015_2024 << ","
                            << media.fitness_2025 << ","
                            << media.beneficio_2025 << ","
                            << media.evaluaciones << ","
                            << media.tiempo_seg << "\n";
            }

            for (auto& par : runs_memeticos) {
                string nombre_alg = par.first;
                ResultadoMedio media = calcularMedia(par.second);

                imprimirFila(nombre_alg, media);

                resumen_csv << ds.nombre << "," << nombre_alg << ","
                            << media.fitness_2015_2024 << ","
                            << media.fitness_2025 << ","
                            << media.beneficio_2025 << ","
                            << media.evaluaciones << ","
                            << media.tiempo_seg << "\n";
            }

            for (auto& par : runs_geneticos_extra) {
                ResultadoExtra extra;
                extra.dataset = ds.nombre;
                extra.algoritmo = par.first;
                extra.media = calcularMedia(par.second);
                resultados_extra.push_back(extra);
            }

            for (auto& par : runs_memeticos_extra) {
                ResultadoExtra extra;
                extra.dataset = ds.nombre;
                extra.algoritmo = par.first;
                extra.media = calcularMedia(par.second);
                resultados_extra.push_back(extra);
            }
        }

        resumen_csv.close();

        cout << "\n##################################################\n";
        cout << "EXPERIMENTOS EXTRA\n";
        cout << "##################################################\n";

        // Las variantes voluntarias se separan de la tabla principal.
        for (size_t d = 0; d < datasets.size(); d++) {
            ConfiguracionDataset ds = datasets[d];

            cout << "\n=============================================================\n";
            cout << "Uniforme y AM-Best1 en " << ds.nombre << " (medias)\n";
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

            for (size_t i = 0; i < resultados_extra.size(); i++) {
                ResultadoExtra extra = resultados_extra[i];
                if (extra.dataset != ds.nombre) continue;

                cout << left
                     << setw(12) << extra.algoritmo
                     << setw(15) << extra.media.fitness_2015_2024
                     << setw(15) << extra.media.fitness_2025
                     << setw(15) << extra.media.beneficio_2025
                     << setw(15) << extra.media.evaluaciones
                     << setw(12) << extra.media.tiempo_seg
                     << "\n";
            }
        }

        // Se generan los CSV necesarios para dibujar las curvas de convergencia.
        GeneticAlgorithm aggBlxConv("AGG", "BLX");
        MemeticAlgorithm amBestConv("BEST", "BLX");

        for (size_t d = 0; d < datasets.size(); d++) {
            ConfiguracionDataset ds = datasets[d];

            string sufijo;
            if (ds.nombre == "Ibex 35") sufijo = "ibex_35";
            else if (ds.nombre == "S&P 100") sufijo = "syp_100";
            else sufijo = "syp_500";

            ofstream conv_csv("resultados/convergencia_" + sufijo + ".csv");
            if (!conv_csv.is_open()) {
                throw runtime_error("No se pudo crear convergencia_" + sufijo + ".csv");
            }
            conv_csv << "algoritmo,evaluaciones,fitness\n";
            conv_csv << fixed << setprecision(6);

            int semilla_conv = cfg.semilla_inicial;

            // Cada bloque escribe la evolución del mejor fitness hasta ese punto.
            Random::seed(semilla_conv);
            {
                PortfolioConvergencia portfolio_random(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "Random");
                randomAlg.optimize(portfolio_random, cfg.eval_random);
            }

            Random::seed(semilla_conv);
            {
                PortfolioConvergencia portfolio_bl(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "BL");
                blAlg.optimize(portfolio_bl, cfg.eval_bl);
            }

            Random::seed(semilla_conv);
            {
                PortfolioConvergencia portfolio_agg(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "AGG-BLX");
                aggBlxConv.optimize(portfolio_agg, cfg.eval_ag);
            }

            Random::seed(semilla_conv);
            {
                PortfolioConvergencia portfolio_am(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "AM-Best");
                amBestConv.optimize(portfolio_am, cfg.eval_am);
            }

            conv_csv.close();
        }
        // Sensibilidad de alpha en BLX usando AGG-BLX.
        vector<double> alphas = {0.1, 0.3, 0.5};

        ofstream alpha_csv("resultados/sensibilidad_alpha.csv");
        if (!alpha_csv.is_open()) {
            throw runtime_error("No se pudo crear sensibilidad_alpha.csv");
        }
        alpha_csv << "dataset,alpha,fitness_medio,fitness_2025_medio,tiempo_seg\n";
        alpha_csv << fixed << setprecision(6);

        for (size_t d = 0; d < datasets.size(); d++) {
            ConfiguracionDataset ds = datasets[d];

            cout << "\n=============================================================\n";
            cout << "Sensibilidad de alpha en " << ds.nombre << " (AGG-BLX)\n";
            cout << "=============================================================\n";
            cout << left
                 << setw(10) << "Alpha"
                 << setw(15) << "2015-2024"
                 << setw(15) << "2025 Fit."
                 << setw(12) << "Tiempo(s)"
                 << "\n";
            cout << fixed << setprecision(3);

            Portfolio portfolio_a(ds.fichero, ds.lo, ds.hi, cfg.lambda);

            DatosPeriodo datos_2025_a = cargarPeriodoCSV(ds.fichero, "2025-01-01", "2025-12-31");
            MetricasPeriodo metricas_2025_a = calcularMetricasPeriodo(datos_2025_a);

            for (size_t a = 0; a < alphas.size(); a++) {
                double alpha = alphas[a];

                GeneticAlgorithm aggAlpha("AGG", "BLX");
                aggAlpha.setAlphaBLX(alpha);

                double suma_fit_train = 0.0;
                double suma_fit_2025 = 0.0;
                double suma_tiempo = 0.0;

                for (int rep = 0; rep < cfg.num_ejecuciones; rep++) {
                    Random::seed(semillas[rep]);

                    auto inicio = high_resolution_clock::now();
                    auto res = aggAlpha.optimize(portfolio_a, cfg.eval_ag);
                    auto fin = high_resolution_clock::now();

                    suma_tiempo += duration<double>(fin - inicio).count();
                    suma_fit_train += res.fitness;

                    auto eval = evaluarSolucionEnPeriodo(res.solution, metricas_2025_a, cfg.lambda);
                    suma_fit_2025 += eval.first;
                }

                double media_train = suma_fit_train / cfg.num_ejecuciones;
                double media_2025  = suma_fit_2025 / cfg.num_ejecuciones;
                double media_tiempo = suma_tiempo / cfg.num_ejecuciones;

                alpha_csv << ds.nombre << "," << alpha << ","
                          << media_train << "," << media_2025 << ","
                          << media_tiempo << "\n";

                cout << left
                     << setw(10) << alpha
                     << setw(15) << media_train
                     << setw(15) << media_2025
                     << setw(12) << media_tiempo
                     << "\n";
            }
        }

        alpha_csv.close();

        // Sensibilidad del tamaño de torneo usando AGG-BLX.
        vector<int> valores_k = {2, 3, 5, 7};

        ofstream torneo_csv("resultados/sensibilidad_torneo.csv");
        if (!torneo_csv.is_open()) {
            throw runtime_error("No se pudo crear sensibilidad_torneo.csv");
        }
        torneo_csv << "dataset,k,fitness_medio,fitness_2025_medio,tiempo_seg\n";
        torneo_csv << fixed << setprecision(6);

        for (size_t d = 0; d < datasets.size(); d++) {
            ConfiguracionDataset ds = datasets[d];

            cout << "\n=============================================================\n";
            cout << "Sensibilidad de k en torneo para " << ds.nombre << " (AGG-BLX)\n";
            cout << "=============================================================\n";
            cout << left
                 << setw(10) << "k"
                 << setw(15) << "2015-2024"
                 << setw(15) << "2025 Fit."
                 << setw(12) << "Tiempo(s)"
                 << "\n";
            cout << fixed << setprecision(3);

            Portfolio portfolio_k(ds.fichero, ds.lo, ds.hi, cfg.lambda);

            DatosPeriodo datos_2025_k = cargarPeriodoCSV(ds.fichero, "2025-01-01", "2025-12-31");
            MetricasPeriodo metricas_2025_k = calcularMetricasPeriodo(datos_2025_k);

            for (size_t k = 0; k < valores_k.size(); k++) {
                int valor_k = valores_k[k];

                GeneticAlgorithm aggTorneo("AGG", "BLX");
                aggTorneo.setKTorneo(valor_k);

                double suma_fit_train = 0.0;
                double suma_fit_2025 = 0.0;
                double suma_tiempo = 0.0;

                for (int rep = 0; rep < cfg.num_ejecuciones; rep++) {
                    Random::seed(semillas[rep]);

                    auto inicio = high_resolution_clock::now();
                    auto res = aggTorneo.optimize(portfolio_k, cfg.eval_ag);
                    auto fin = high_resolution_clock::now();

                    suma_tiempo += duration<double>(fin - inicio).count();
                    suma_fit_train += res.fitness;

                    auto eval = evaluarSolucionEnPeriodo(res.solution, metricas_2025_k, cfg.lambda);
                    suma_fit_2025 += eval.first;
                }

                double media_train = suma_fit_train / cfg.num_ejecuciones;
                double media_2025  = suma_fit_2025 / cfg.num_ejecuciones;
                double media_tiempo = suma_tiempo / cfg.num_ejecuciones;

                torneo_csv << ds.nombre << "," << valor_k << ","
                            << media_train << "," << media_2025 << ","
                            << media_tiempo << "\n";

                cout << left
                     << setw(10) << valor_k
                     << setw(15) << media_train
                     << setw(15) << media_2025
                     << setw(12) << media_tiempo
                     << "\n";
            }
        }

        torneo_csv.close();

        cout << "\nFicheros generados:\n";
        cout << "- resultados/resultados_medias.csv\n";
        cout << "- resultados/semillas_utilizadas.csv\n";
        cout << "- resultados/configuracion_usada.txt\n";
        cout << "- resultados/boxplot_ibex_35.csv\n";
        cout << "- resultados/boxplot_syp_100.csv\n";
        cout << "- resultados/boxplot_syp_500.csv\n";
        cout << "- resultados/convergencia_ibex_35.csv\n";
        cout << "- resultados/convergencia_syp_100.csv\n";
        cout << "- resultados/convergencia_syp_500.csv\n";
        cout << "- resultados/sensibilidad_alpha.csv\n";
        cout << "- resultados/sensibilidad_torneo.csv\n";
        cout << "\nSi quieres cambiar parametros, ejecuta por ejemplo:\n";
        cout << "./main config.txt\n";
        return 0;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
