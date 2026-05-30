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
#include "randomsearch.h"
#include "greedy.h"
#include "localsearch.h"
#include "memetic.h"
#include "simulatedannealing.h"
#include "bmb.h"
#include "grasp.h"
#include "ils.h"
#include "ilses.h"
#include "portfolio_convergencia.h"
#include "random.hpp"

using namespace std;
using namespace std::chrono;

// Datos fijos de cada conjunto: nombre, fichero, restricciones y CSV asociado.
struct ConfiguracionDataset {
    string nombre;
    string fichero;
    double lo;
    double hi;
    string fichero_boxplot;
};

// Guarda las filas de rendimientos de un periodo concreto.
struct DatosPeriodo {
    vector<vector<double>> mu;
};

// Metricas precalculadas para no repetir trabajo al evaluar en 2025.
struct MetricasPeriodo {
    vector<double> beneficio_interno;
    vector<vector<double>> cov;
};

// Resultado de una ejecucion individual de un algoritmo.
struct ResultadoEjecucion {
    double fitness_2015_2024 = 0.0;
    double fitness_2025 = 0.0;
    double beneficio_2025 = 0.0;
    double tiempo_seg = 0.0;
    int evaluaciones = 0;
};

// Media de las ejecuciones de un algoritmo en un dataset.
struct ResultadoMedio {
    double fitness_2015_2024 = 0.0;
    double fitness_2025 = 0.0;
    double beneficio_2025 = 0.0;
    double tiempo_seg = 0.0;
    double evaluaciones = 0.0;
};

// Parametros del experimento. Si no se pasa config.txt se usan estos valores.
struct ConfiguracionExperimento {
    double lambda = 500.0;

    int num_ejecuciones = 50;
    int semilla_inicial = 0;

    int eval_greedy = 1;
    int eval_random = 10000;
    int eval_bl = 10000;
    int eval_am = 10000;

    int eval_es = 10000;
    int eval_bmb = 10000;
    int eval_bmb10 = 10000;
    int eval_grasp = 10000;
    int eval_ils = 10000;
    int eval_ilses = 10000;

    int iteraciones_bmb = 5;
    int iteraciones_bmb10 = 10;
    int iteraciones_grasp = 5;
    int iteraciones_ils = 5;
    int iteraciones_ilses = 5;

    int eval_bl_bmb = 2000;
    int eval_bl_bmb10 = 1000;
    int eval_bl_grasp = 2000;
    int eval_bl_ils = 2000;
    int eval_es_ilses = 2000;
    int grasp_lrc = 5;

    double es_phi = 0.3;
    double es_mu = 0.2;
    double es_tf = 1e-3;
};

// Quita espacios al principio y al final de una cadena.
string trim(const string& s) {
    size_t ini = s.find_first_not_of(" \t\r\n");
    if (ini == string::npos) return "";

    size_t fin = s.find_last_not_of(" \t\r\n");
    return s.substr(ini, fin - ini + 1);
}

// Lee config.txt y sobrescribe solo las claves que aparezcan en el fichero.
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
        else if (clave == "semilla_inicial") cfg.semilla_inicial = stoi(valor);

        else if (clave == "eval_greedy") cfg.eval_greedy = stoi(valor);
        else if (clave == "eval_random") cfg.eval_random = stoi(valor);
        else if (clave == "eval_bl") cfg.eval_bl = stoi(valor);
        else if (clave == "eval_am") cfg.eval_am = stoi(valor);

        else if (clave == "eval_es") cfg.eval_es = stoi(valor);
        else if (clave == "eval_bmb") cfg.eval_bmb = stoi(valor);
        else if (clave == "eval_bmb10") cfg.eval_bmb10 = stoi(valor);
        else if (clave == "eval_grasp") cfg.eval_grasp = stoi(valor);
        else if (clave == "eval_ils") cfg.eval_ils = stoi(valor);
        else if (clave == "eval_ilses") cfg.eval_ilses = stoi(valor);

        else if (clave == "iteraciones_bmb") cfg.iteraciones_bmb = stoi(valor);
        else if (clave == "iteraciones_bmb10") cfg.iteraciones_bmb10 = stoi(valor);
        else if (clave == "iteraciones_grasp") cfg.iteraciones_grasp = stoi(valor);
        else if (clave == "iteraciones_ils") cfg.iteraciones_ils = stoi(valor);
        else if (clave == "iteraciones_ilses") cfg.iteraciones_ilses = stoi(valor);

        else if (clave == "eval_bl_bmb") cfg.eval_bl_bmb = stoi(valor);
        else if (clave == "eval_bl_bmb10") cfg.eval_bl_bmb10 = stoi(valor);
        else if (clave == "eval_bl_grasp") cfg.eval_bl_grasp = stoi(valor);
        else if (clave == "eval_bl_ils") cfg.eval_bl_ils = stoi(valor);
        else if (clave == "eval_es_ilses") cfg.eval_es_ilses = stoi(valor);
        else if (clave == "grasp_lrc") cfg.grasp_lrc = stoi(valor);

        else if (clave == "es_phi") cfg.es_phi = stod(valor);
        else if (clave == "es_mu") cfg.es_mu = stod(valor);
        else if (clave == "es_tf") cfg.es_tf = stod(valor);
    }

    return cfg;
}

// Carga solo las filas del CSV cuyas fechas caen dentro del intervalo indicado.
DatosPeriodo cargarPeriodoCSV(const string& fichero, const string& fecha_inicio, const string& fecha_fin) {
    DatosPeriodo datos;
    ifstream file(fichero);

    if (!file.is_open()) {
        throw runtime_error("No se pudo abrir el fichero: " + fichero);
    }

    string line;
    getline(file, line); // cabecera

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string celda;
        string fecha;
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

// Precalcula beneficio interno y covarianza para evaluar carteras en un periodo.
MetricasPeriodo calcularMetricasPeriodo(const DatosPeriodo& datos) {
    MetricasPeriodo metricas;

    if (datos.mu.empty()) {
        return metricas;
    }

    int T = (int)datos.mu.size();
    int n = (int)datos.mu[0].size();

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

// Evalua una cartera en un periodo distinto al usado para entrenar.
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

// Calcula la media de todas las repeticiones guardadas.
ResultadoMedio calcularMedia(const vector<ResultadoEjecucion>& runs) {
    ResultadoMedio media;

    if (runs.empty()) return media;

    for (const auto& r : runs) {
        media.fitness_2015_2024 += r.fitness_2015_2024;
        media.fitness_2025 += r.fitness_2025;
        media.beneficio_2025 += r.beneficio_2025;
        media.evaluaciones += r.evaluaciones;
        media.tiempo_seg += r.tiempo_seg;
    }

    double total = (double)runs.size();

    media.fitness_2015_2024 /= total;
    media.fitness_2025 /= total;
    media.beneficio_2025 /= total;
    media.evaluaciones /= total;
    media.tiempo_seg /= total;

    return media;
}

// Imprime una fila sencilla para revisar resultados por terminal.
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

// Ejecuta un algoritmo en 2015-2024 y luego evalua la cartera obtenida en 2025.
ResultadoEjecucion ejecutarAlgoritmo(
    MH<double>& algoritmo,
    Portfolio& portfolio_train,
    const MetricasPeriodo& metricas_2025,
    double lambda,
    int maxevals
) {
    auto inicio = high_resolution_clock::now();
    auto res = algoritmo.optimize(portfolio_train, maxevals);
    auto fin = high_resolution_clock::now();

    ResultadoEjecucion r;

    r.tiempo_seg = duration<double>(fin - inicio).count();
    r.fitness_2015_2024 = res.fitness;
    r.evaluaciones = res.evaluations;

    auto eval_2025 = evaluarSolucionEnPeriodo(res.solution, metricas_2025, lambda);
    r.fitness_2025 = eval_2025.first;
    r.beneficio_2025 = eval_2025.second;

    return r;
}

int main(int argc, char* argv[]) {
    try {
        ConfiguracionExperimento cfg;

        // Si se pasa config.txt, se usan sus parametros. Si no, quedan los valores por defecto.
        if (argc >= 2) {
            cfg = leerConfiguracion(argv[1]);
        }

        // Datasets usados en la practica, con sus limites lo/hi.
        vector<ConfiguracionDataset> datasets = {
            {"Ibex 35", "datos_portfolio_2526/ibex_35.csv", 0.005, 0.08, "resultados/boxplot_ibex_35.csv"},
            {"S&P 100", "datos_portfolio_2526/syp_100.csv", 0.005, 0.05, "resultados/boxplot_syp_100.csv"},
            {"S&P 500", "datos_portfolio_2526/syp_500.csv", 0.005, 0.02, "resultados/boxplot_syp_500.csv"}
        };

        // Mismas semillas para todos los algoritmos, asi las comparaciones son mas justas.
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

        // Guardamos la configuracion real usada para poder repetir el experimento.
        ofstream config_txt("resultados/configuracion_usada.txt");
        if (!config_txt.is_open()) {
            throw runtime_error("No se pudo crear resultados/configuracion_usada.txt");
        }

        config_txt << "lambda=" << cfg.lambda << "\n";
        config_txt << "num_ejecuciones=" << cfg.num_ejecuciones << "\n";
        config_txt << "semilla_inicial=" << cfg.semilla_inicial << "\n";
        config_txt << "eval_greedy=" << cfg.eval_greedy << "\n";
        config_txt << "eval_random=" << cfg.eval_random << "\n";
        config_txt << "eval_bl=" << cfg.eval_bl << "\n";
        config_txt << "eval_am=" << cfg.eval_am << "\n";
        config_txt << "eval_es=" << cfg.eval_es << "\n";
        config_txt << "eval_bmb=" << cfg.eval_bmb << "\n";
        config_txt << "eval_bmb10=" << cfg.eval_bmb10 << "\n";
        config_txt << "eval_grasp=" << cfg.eval_grasp << "\n";
        config_txt << "eval_ils=" << cfg.eval_ils << "\n";
        config_txt << "eval_ilses=" << cfg.eval_ilses << "\n";
        config_txt << "iteraciones_bmb=" << cfg.iteraciones_bmb << "\n";
        config_txt << "iteraciones_bmb10=" << cfg.iteraciones_bmb10 << "\n";
        config_txt << "iteraciones_grasp=" << cfg.iteraciones_grasp << "\n";
        config_txt << "iteraciones_ils=" << cfg.iteraciones_ils << "\n";
        config_txt << "iteraciones_ilses=" << cfg.iteraciones_ilses << "\n";
        config_txt << "eval_bl_bmb=" << cfg.eval_bl_bmb << "\n";
        config_txt << "eval_bl_bmb10=" << cfg.eval_bl_bmb10 << "\n";
        config_txt << "eval_bl_grasp=" << cfg.eval_bl_grasp << "\n";
        config_txt << "eval_bl_ils=" << cfg.eval_bl_ils << "\n";
        config_txt << "eval_es_ilses=" << cfg.eval_es_ilses << "\n";
        config_txt << "grasp_lrc=" << cfg.grasp_lrc << "\n";
        config_txt << "es_phi=" << cfg.es_phi << "\n";
        config_txt << "es_mu=" << cfg.es_mu << "\n";
        config_txt << "es_tf=" << cfg.es_tf << "\n";
        config_txt.close();

        ofstream resumen_csv("resultados/resultados_medias.csv");
        if (!resumen_csv.is_open()) {
            throw runtime_error("No se pudo crear resultados/resultados_medias.csv");
        }

        resumen_csv << "dataset,algoritmo,fitness_2015_2024,fitness_2025,beneficio_2025,evaluaciones,tiempo_seg\n";
        resumen_csv << fixed << setprecision(6);

        // Algoritmos base y referencias comparadas en la memoria.
        RandomSearch<double> randomAlg;
        GreedySearch greedyAlg;
        LocalSearch blAlg;

        // Mejor algoritmo de la P2 usado como referencia.
        MemeticAlgorithm amBest("BEST", "BLX");

        // Algoritmos principales de P3 y variantes voluntarias.
        SimulatedAnnealing esAlg(cfg.es_phi, cfg.es_mu, cfg.es_tf);
        BMB bmbAlg(cfg.iteraciones_bmb, cfg.eval_bl_bmb);
        BMB bmb10Alg(cfg.iteraciones_bmb10, cfg.eval_bl_bmb10);
        GRASP graspAlg(cfg.iteraciones_grasp, cfg.eval_bl_grasp, cfg.grasp_lrc);
        ILS ilsAlg(cfg.iteraciones_ils, cfg.eval_bl_ils);
        ILS ils10Alg(cfg.iteraciones_ils, cfg.eval_bl_ils, 0.10);
        ILS ils30Alg(cfg.iteraciones_ils, cfg.eval_bl_ils, 0.30);
        ILSES ilsesAlg(cfg.iteraciones_ilses, cfg.eval_es_ilses);
        ILSES ilses10Alg(cfg.iteraciones_ilses, cfg.eval_es_ilses, 0.10);
        ILSES ilses30Alg(cfg.iteraciones_ilses, cfg.eval_es_ilses, 0.30);
        ilsesAlg.setParametrosES(cfg.es_phi, cfg.es_mu, cfg.es_tf);
        ilses10Alg.setParametrosES(cfg.es_phi, cfg.es_mu, cfg.es_tf);
        ilses30Alg.setParametrosES(cfg.es_phi, cfg.es_mu, cfg.es_tf);

        for (const auto& ds : datasets) {
            cout << "\n##################################################\n";
            cout << "DATASET: " << ds.nombre << "\n";
            cout << "##################################################\n";

            // El entrenamiento usa 2015-2024 dentro de la clase Portfolio.
            Portfolio portfolio_train(ds.fichero, ds.lo, ds.hi, cfg.lambda);

            // Las soluciones se prueban despues fuera de muestra, en 2025.
            DatosPeriodo datos_2025 = cargarPeriodoCSV(ds.fichero, "2025-01-01", "2025-12-31");
            MetricasPeriodo metricas_2025 = calcularMetricasPeriodo(datos_2025);

            map<string, vector<ResultadoEjecucion>> runs;

            ofstream boxplot_csv(ds.fichero_boxplot);
            if (!boxplot_csv.is_open()) {
                throw runtime_error("No se pudo crear " + ds.fichero_boxplot);
            }

            boxplot_csv << "algoritmo,fitness_2015_2024\n";
            boxplot_csv << fixed << setprecision(6);

            for (int rep = 0; rep < cfg.num_ejecuciones; rep++) {
                int semilla = semillas[rep];

                // Greedy determinista.
                Random::seed(semilla);
                ResultadoEjecucion r_greedy = ejecutarAlgoritmo(
                    greedyAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_greedy
                );
                runs["Greedy"].push_back(r_greedy);
                boxplot_csv << "Greedy," << r_greedy.fitness_2015_2024 << "\n";

                // Busqueda aleatoria de referencia.
                Random::seed(semilla);
                ResultadoEjecucion r_random = ejecutarAlgoritmo(
                    randomAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_random
                );
                runs["Random"].push_back(r_random);

                // BL de la práctica 1.
                Random::seed(semilla);
                ResultadoEjecucion r_bl = ejecutarAlgoritmo(
                    blAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_bl
                );
                runs["BL"].push_back(r_bl);
                boxplot_csv << "BL," << r_bl.fitness_2015_2024 << "\n";

                // AM-Best de la practica 2.
                Random::seed(semilla);
                ResultadoEjecucion r_am = ejecutarAlgoritmo(
                    amBest, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_am
                );
                runs["AM-Best"].push_back(r_am);
                boxplot_csv << "AM-Best," << r_am.fitness_2015_2024 << "\n";

                // BMB: varias BL desde soluciones aleatorias independientes.
                Random::seed(semilla);
                ResultadoEjecucion r_bmb = ejecutarAlgoritmo(
                    bmbAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_bmb
                );
                runs["BMB"].push_back(r_bmb);
                boxplot_csv << "BMB," << r_bmb.fitness_2015_2024 << "\n";

                // BMB-10: mas arranques, pero menos evaluaciones por BL.
                Random::seed(semilla);
                ResultadoEjecucion r_bmb10 = ejecutarAlgoritmo(
                    bmb10Alg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_bmb10
                );
                runs["BMB-10"].push_back(r_bmb10);
                boxplot_csv << "BMB-10," << r_bmb10.fitness_2015_2024 << "\n";

                // GRASP: construccion semi-voraz y despues BL.
                Random::seed(semilla);
                ResultadoEjecucion r_grasp = ejecutarAlgoritmo(
                    graspAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_grasp
                );
                runs["GRASP"].push_back(r_grasp);
                boxplot_csv << "GRASP," << r_grasp.fitness_2015_2024 << "\n";

                // ILS con BL y mutacion del 20%.
                Random::seed(semilla);
                ResultadoEjecucion r_ils = ejecutarAlgoritmo(
                    ilsAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_ils
                );
                runs["ILS-BL"].push_back(r_ils);
                boxplot_csv << "ILS-BL," << r_ils.fitness_2015_2024 << "\n";

                // Variante con mutacion del 10%.
                Random::seed(semilla);
                ResultadoEjecucion r_ils10 = ejecutarAlgoritmo(
                    ils10Alg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_ils
                );
                runs["ILS-10"].push_back(r_ils10);
                boxplot_csv << "ILS-10," << r_ils10.fitness_2015_2024 << "\n";

                // Variante con mutacion del 30%.
                Random::seed(semilla);
                ResultadoEjecucion r_ils30 = ejecutarAlgoritmo(
                    ils30Alg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_ils
                );
                runs["ILS-30"].push_back(r_ils30);
                boxplot_csv << "ILS-30," << r_ils30.fitness_2015_2024 << "\n";

                // Enfriamiento Simulado.
                Random::seed(semilla);
                ResultadoEjecucion r_es = ejecutarAlgoritmo(
                    esAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_es
                );
                runs["ES"].push_back(r_es);
                boxplot_csv << "ES," << r_es.fitness_2015_2024 << "\n";

                // ILS hibridado con ES y mutacion del 20%.
                Random::seed(semilla);
                ResultadoEjecucion r_ilses = ejecutarAlgoritmo(
                    ilsesAlg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_ilses
                );
                runs["ILS-ES"].push_back(r_ilses);
                boxplot_csv << "ILS-ES," << r_ilses.fitness_2015_2024 << "\n";

                // Variante ILS-ES con mutacion del 10%.
                Random::seed(semilla);
                ResultadoEjecucion r_ilses10 = ejecutarAlgoritmo(
                    ilses10Alg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_ilses
                );
                runs["ILS-ES-10"].push_back(r_ilses10);
                boxplot_csv << "ILS-ES-10," << r_ilses10.fitness_2015_2024 << "\n";

                // Variante ILS-ES con mutacion del 30%.
                Random::seed(semilla);
                ResultadoEjecucion r_ilses30 = ejecutarAlgoritmo(
                    ilses30Alg, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_ilses
                );
                runs["ILS-ES-30"].push_back(r_ilses30);
                boxplot_csv << "ILS-ES-30," << r_ilses30.fitness_2015_2024 << "\n";
            }

            boxplot_csv.close();

            vector<string> orden_principal = {
                "Greedy",
                "Random",
                "BL",
                "AM-Best",
                "BMB",
                "ILS-BL",
                "ES",
                "ILS-ES"
            };

            vector<string> orden_variantes = {
                "BMB-10",
                "GRASP",
                "ILS-10",
                "ILS-30",
                "ILS-ES-10",
                "ILS-ES-30"
            };

            vector<string> orden_csv = orden_principal;
            orden_csv.insert(orden_csv.end(), orden_variantes.begin(), orden_variantes.end());

            auto imprimirCabeceraTabla = []() {
                cout << left
                     << setw(12) << "Algoritmo"
                     << setw(15) << "2015-2024"
                     << setw(15) << "2025 Fit."
                     << setw(15) << "2025 Benef."
                     << setw(15) << "Evaluaciones"
                     << setw(12) << "Tiempo(s)"
                     << "\n";
            };

            auto imprimirTabla = [&](const vector<string>& orden_tabla) {
                imprimirCabeceraTabla();
                cout << fixed << setprecision(3);

                for (const string& nombre_alg : orden_tabla) {
                    ResultadoMedio media = calcularMedia(runs[nombre_alg]);
                    imprimirFila(nombre_alg, media);
                }
            };

            cout << "\n=============================================================\n";
            cout << "Resultados principales de " << ds.nombre << " (medias)\n";
            cout << "=============================================================\n";
            imprimirTabla(orden_principal);

            cout << "\n=============================================================\n";
            cout << "Variantes extra de " << ds.nombre << " (medias)\n";
            cout << "=============================================================\n";
            imprimirTabla(orden_variantes);

            for (const string& nombre_alg : orden_csv) {
                ResultadoMedio media = calcularMedia(runs[nombre_alg]);

                resumen_csv << ds.nombre << "," << nombre_alg << ","
                            << media.fitness_2015_2024 << ","
                            << media.fitness_2025 << ","
                            << media.beneficio_2025 << ","
                            << media.evaluaciones << ","
                            << media.tiempo_seg << "\n";
            }
        }

        resumen_csv.close();

        // Experimento extra: sensibilidad de ES respecto a phi.
        ofstream sensibilidad_phi("resultados/sensibilidad_es_phi.csv");
        sensibilidad_phi << "dataset,phi,fitness_2015_2024,fitness_2025,tiempo_seg\n";
        sensibilidad_phi << fixed << setprecision(6);

        vector<double> valores_phi = {0.1, 0.3, 0.5};
        for (const auto& ds : datasets) {
            // El entrenamiento usa 2015-2024 dentro de la clase Portfolio.
            Portfolio portfolio_train(ds.fichero, ds.lo, ds.hi, cfg.lambda);
            // Las soluciones se prueban despues fuera de muestra, en 2025.
            DatosPeriodo datos_2025 = cargarPeriodoCSV(ds.fichero, "2025-01-01", "2025-12-31");
            MetricasPeriodo metricas_2025 = calcularMetricasPeriodo(datos_2025);

            for (double phi_actual : valores_phi) {
                vector<ResultadoEjecucion> runs_phi;

                for (int rep = 0; rep < cfg.num_ejecuciones; rep++) {
                    Random::seed(semillas[rep]);
                    SimulatedAnnealing es(phi_actual, cfg.es_mu, cfg.es_tf);
                    runs_phi.push_back(ejecutarAlgoritmo(es, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_es));
                }

                ResultadoMedio media = calcularMedia(runs_phi);
                sensibilidad_phi << ds.nombre << "," << phi_actual << ","
                                 << media.fitness_2015_2024 << ","
                                 << media.fitness_2025 << ","
                                 << media.tiempo_seg << "\n";
            }
        }
        sensibilidad_phi.close();

        // Experimento extra: sensibilidad de ES respecto a mu.
        ofstream sensibilidad_mu("resultados/sensibilidad_es_mu.csv");
        sensibilidad_mu << "dataset,mu,fitness_2015_2024,fitness_2025,tiempo_seg\n";
        sensibilidad_mu << fixed << setprecision(6);

        vector<double> valores_mu = {0.1, 0.2, 0.4};
        for (const auto& ds : datasets) {
            // El entrenamiento usa 2015-2024 dentro de la clase Portfolio.
            Portfolio portfolio_train(ds.fichero, ds.lo, ds.hi, cfg.lambda);
            // Las soluciones se prueban despues fuera de muestra, en 2025.
            DatosPeriodo datos_2025 = cargarPeriodoCSV(ds.fichero, "2025-01-01", "2025-12-31");
            MetricasPeriodo metricas_2025 = calcularMetricasPeriodo(datos_2025);

            for (double mu_actual : valores_mu) {
                vector<ResultadoEjecucion> runs_mu;

                for (int rep = 0; rep < cfg.num_ejecuciones; rep++) {
                    Random::seed(semillas[rep]);
                    SimulatedAnnealing es(cfg.es_phi, mu_actual, cfg.es_tf);
                    runs_mu.push_back(ejecutarAlgoritmo(es, portfolio_train, metricas_2025, cfg.lambda, cfg.eval_es));
                }

                ResultadoMedio media = calcularMedia(runs_mu);
                sensibilidad_mu << ds.nombre << "," << mu_actual << ","
                                << media.fitness_2015_2024 << ","
                                << media.fitness_2025 << ","
                                << media.tiempo_seg << "\n";
            }
        }
        sensibilidad_mu.close();

        // Experimento extra: curvas de convergencia de los metodos principales.
        for (const auto& ds : datasets) {
            string sufijo = "ibex_35";
            if (ds.nombre == "S&P 100") sufijo = "syp_100";
            else if (ds.nombre == "S&P 500") sufijo = "syp_500";

            ofstream conv_csv("resultados/convergencia_" + sufijo + ".csv");
            conv_csv << "algoritmo,evaluaciones,fitness\n";
            conv_csv << fixed << setprecision(6);

            int semilla = cfg.semilla_inicial;

            Random::seed(semilla);
            {
                PortfolioConvergencia p(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "Random");
                RandomSearch<double> alg;
                alg.optimize(p, cfg.eval_random);
            }

            Random::seed(semilla);
            {
                PortfolioConvergencia p(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "BL");
                LocalSearch alg;
                alg.optimize(p, cfg.eval_bl);
            }

            Random::seed(semilla);
            {
                PortfolioConvergencia p(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "AM-Best");
                MemeticAlgorithm alg("BEST", "BLX");
                alg.optimize(p, cfg.eval_am);
            }

            Random::seed(semilla);
            {
                PortfolioConvergencia p(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "BMB");
                BMB alg(cfg.iteraciones_bmb, cfg.eval_bl_bmb);
                alg.optimize(p, cfg.eval_bmb);
            }

            Random::seed(semilla);
            {
                PortfolioConvergencia p(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "ILS-BL");
                ILS alg(cfg.iteraciones_ils, cfg.eval_bl_ils);
                alg.optimize(p, cfg.eval_ils);
            }

            Random::seed(semilla);
            {
                PortfolioConvergencia p(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "ES");
                SimulatedAnnealing alg(cfg.es_phi, cfg.es_mu, cfg.es_tf);
                alg.optimize(p, cfg.eval_es);
            }

            Random::seed(semilla);
            {
                PortfolioConvergencia p(ds.fichero, ds.lo, ds.hi, cfg.lambda, conv_csv, "ILS-ES");
                ILSES alg(cfg.iteraciones_ilses, cfg.eval_es_ilses);
                alg.setParametrosES(cfg.es_phi, cfg.es_mu, cfg.es_tf);
                alg.optimize(p, cfg.eval_ilses);
            }

            conv_csv.close();
        }

        cout << "\nFicheros generados:\n";
        cout << "- resultados/resultados_medias.csv\n";
        cout << "- resultados/semillas_utilizadas.csv\n";
        cout << "- resultados/configuracion_usada.txt\n";
        cout << "- resultados/boxplot_ibex_35.csv\n";
        cout << "- resultados/boxplot_syp_100.csv\n";
        cout << "- resultados/boxplot_syp_500.csv\n";
        cout << "- resultados/sensibilidad_es_phi.csv\n";
        cout << "- resultados/sensibilidad_es_mu.csv\n";
        cout << "- resultados/convergencia_ibex_35.csv\n";
        cout << "- resultados/convergencia_syp_100.csv\n";
        cout << "- resultados/convergencia_syp_500.csv\n";

        cout << "\nPara ejecutar con configuración personalizada:\n";
        cout << "./main config.txt\n";

        return 0;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
