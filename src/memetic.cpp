#include "../inc/memetic.h"
#include "../inc/portfolio.h"
#include "../common/random.hpp"

#include <algorithm>
#include <cmath>

using namespace std;

// Constructor de MemeticAlgorithm, inicializa el modelo, el tipo de cruce y los parámetros comunes.
MemeticAlgorithm::MemeticAlgorithm(const string& tipo, const string& cruce) : MHDouble() {

    tipo_am = tipo;             // Tipo de modelo: "ALL", "RAND", "BEST" o "BEST1"
    tipo_cruce = cruce;         // Tipo de cruce: "ARIT" o "BLX"
    tamaño_poblacion = 100;     // Tamaño de 100 cromosomas
    evaluaciones_max = 10000;   // Tamaño máximo de evaluaciones
    k_torneo = 3;               // Torneo con 3 individuos
    prob_cruce = 0.8;           // Probabilidad de cruce por pareja
    prob_mutacion = 0.1;        // Probabilidad de mutación por individuo
    alpha_BLX = 0.3;            // Parámetro α del cruce BLX-α 
}

// Método que llama a ejecutar el algoritmo memético (para mayor encapsulación)
ResultMH<double> MemeticAlgorithm::optimize(Problem<double>& problema, int evaluaciones) {
    evaluaciones_max = evaluaciones;
    return ejecutarAM(problema);
}

// Método que genera la población inicial con soluciones aleatorias del problema.
vector<MemeticAlgorithm::Individuo> MemeticAlgorithm::inicializarPoblacion(ProblemDouble& problema, int& evaluaciones) {

    vector<Individuo> poblacion; // Creamos la población vacía

    // Generamos cada individuo de la población aleatoriamente
    for (int i = 0; i < tamaño_poblacion; i++) { 
        Individuo individuo;
        individuo.sol = problema.createSolution();
        individuo.fitness = problema.fitness(individuo.sol);
        evaluaciones++;
        poblacion.push_back(individuo);
    }

    return poblacion;
}

// Selección por torneo de tamaño k=3, se eligen k individuos aleatorios, devolviendo el mejor de esos k individuos
int MemeticAlgorithm::seleccionarPorTorneo(const vector<Individuo>& poblacion) {

    int mejor_individuo = -1;

    for (int i = 0; i < k_torneo; i++) {
        int actual = Random::get(0, (int)poblacion.size() - 1); // Escogemos aleatoriamente un individuo

        if ((mejor_individuo == -1) || (poblacion[actual].fitness > poblacion[mejor_individuo].fitness)) { // Mantenemos el de mayor fitness
            mejor_individuo = actual;
        }
    }

    return mejor_individuo;
}

// Método que devuelve el índice del mejor individuo, es decir, el de mayor fitness
int MemeticAlgorithm::mejorIndividuo(const vector<Individuo>& poblacion) const {

    int mejor_individuo = -1;

    for (int i = 0; i < (int)poblacion.size(); i++) {
        if ((mejor_individuo == -1) || (poblacion[i].fitness > poblacion[mejor_individuo].fitness)) {
            mejor_individuo = i;
        }
    }

    return mejor_individuo;
}

// Método que devuelve el índice del peor individuo, es decir, el de menor fitness
int MemeticAlgorithm::peorIndividuo(const vector<Individuo>& poblacion) const {

    int peor_individuo = -1;

    for (int i = 0; i < (int)poblacion.size(); i++) {
        if (peor_individuo == -1 || poblacion[i].fitness < poblacion[peor_individuo].fitness) {
            peor_individuo = i;
        }
    }

    return peor_individuo;
}

// Método que implementa el Cruce Aritmético
void MemeticAlgorithm::cruceAritmetico(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2) {

    size_t n = padre1.size();

    // Reserva el tamaño de los hijos antes de asignar cada gen
    hijo1.resize(n);
    hijo2.resize(n);


    for (size_t i = 0; i < n; i++) {
        // Elegimos el peso de forma aleatoria para combinar ambos padres
        double alpha = Random::get<double>(0.0, 1.0);
        hijo1[i] = alpha * padre1[i] + (1.0 - alpha) * padre2[i];
        hijo2[i] = alpha * padre2[i] + (1.0 - alpha) * padre1[i];
    }
}

// Método que implementa EL Cruce BLX-alpha, con alpha = 0.3
void MemeticAlgorithm::cruceBLX(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2, double alpha) {

    size_t n = padre1.size();

    // Reserva el tamaño de los hijos antes de asignar cada gen
    hijo1.resize(n);
    hijo2.resize(n);

    for (size_t i = 0; i < n; i++) {
        // Calcula el intervalo ampliado entre ambos padres
        double C_min = min(padre1[i], padre2[i]);
        double C_max = max(padre1[i], padre2[i]);
        double I = C_max - C_min;

        double h_min = C_min - alpha * I;
        double h_max = C_max + alpha * I;

        // Genera cada gen dentro del intervalo BLX
        hijo1[i] = Random::get<double>(h_min, h_max);
        hijo2[i] = Random::get<double>(h_min, h_max);
    }
}

// Operador de mutación por intercambio
void MemeticAlgorithm::mutar(tSolution<double>& sol, ProblemDouble& problema) {
    
    Portfolio& portfolio = dynamic_cast<Portfolio&>(problema);

    double lo = portfolio.getLo();
    double hi = portfolio.getHi();
    int n = (int)sol.size();

    // Selecciono dos índices aleatorios
    int i = Random::get(0, n - 1);
    int j = Random::get(0, n - 1);

    // Si i no puede transferir nada, la mutación se cancela
    if (sol[i] <= 0.0) return;

    // REDUCIMOS LA TRANSFERENCIA: Calculo el 15% del valor para intercambiar, y asigno esa cantidad o lo que quepa en j
    double cantidad = 0.15 * sol[i];
    cantidad = min(cantidad, hi - sol[j]);

    // Si i quedara entre 0 y lo, reducimos la cantidad para dejar i en lo. Se respeta que es menor que el 15%, y no viole los límites
    if (sol[i] - cantidad > 0.0 && sol[i] - cantidad < lo) {
        cantidad = sol[i] - lo;
    }

    // Si tras ajustarlo, no se mueve puede mover ninguna cantidad, se cancela la mutación
    if (cantidad <= 0.0) return;

    // Si j estaba a 0, al activarlo debe llegar al mínimo permitido, si no también se cancela la mutación
    if (sol[j] == 0.0 && cantidad < lo) return;

    sol[i] -= cantidad;
    sol[j] += cantidad;
}

// Genera todos los pares (i,j) con i != j que forman el entorno de la solución
vector<pair<int,int>> MemeticAlgorithm::generarEntorno(size_t n) {
    vector<pair<int,int>> pares;

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            if (i != j) {
                pares.push_back({(int)i, (int)j});
            }
        }
    }

    return pares;
}

// Genera un vecino moviendo un porcentaje del peso de i hacia j, reduciendo la transferencia si es necesario
bool MemeticAlgorithm::generarVecino(const tSolution<double>& actual, tSolution<double>& vecino, int i, int j, double ratio, double lo, double hi) {


    // Si i no tiene nada que ceder, no hay vecino
    if (actual[i] <= 0.0) return false;

    // Cantidad máxima que se quiere mover
    double cantidad = ratio * actual[i];

    // Si j no puede recibir tanto, movemos solo lo que quepa hasta hi
    cantidad = min(cantidad, hi - actual[j]);

    // Si i quedaría entre 0 y lo, reducimos la cantidad para dejar i en lo.
    // Así nunca movemos más del ratio indicado.
    if (actual[i] - cantidad > 0.0 && actual[i] - cantidad < lo) {
        cantidad = actual[i] - lo;
    }

    if (cantidad <= 0.0) return false;

    // Si j estaba a 0, al activarlo debe llegar al mínimo permitido
    if (actual[j] == 0.0 && cantidad < lo) return false;

    // Solo ahora, que el vecino es válido, copiamos y aplicamos
    vecino = actual;
    vecino[i] -= cantidad;
    vecino[j] += cantidad;

    return true;
}

// Búsqueda Local del AM: igual que la BL de P1, pero moviendo un 15%
void MemeticAlgorithm::busquedaLocalSuave(tSolution<double>& sol, double& fitness_sol, ProblemDouble& problema, int& evaluaciones) {

    Portfolio& portfolio = dynamic_cast<Portfolio&>(problema);
    double lo = portfolio.getLo();
    double hi = portfolio.getHi();
    double ratio = 0.15; // porcentaje que se mueve de una posición a otra (15% en vez del 40% de P1)
    int n = (int)sol.size();

    int evals_bl = 0;

    // El entorno solo depende de n, así que se genera una vez y se reutiliza
    static vector<pair<int,int>> entorno;
    static int n_entorno = -1;

    if (n_entorno != n) {
        entorno = generarEntorno(n);
        n_entorno = n;
    }

    // Generamos todos los pares (i,j) con i != j que forman el entorno de la solución
    vector<pair<int,int>>& pares = entorno;

    // Repetimos mientras queden evaluaciones disponibles (máximo 100 por llamada)
    while (evals_bl < 100 && evaluaciones < evaluaciones_max) {
        
        // Barajamos los pares aleatoriamente
        shuffle(pares.begin(), pares.end(), Random::get_engine());

        bool mejora = false;

        // Recorremos los vecinos en orden aleatorio
        for (int k = 0; k < (int)pares.size() && evals_bl < 100 && evaluaciones < evaluaciones_max; k++) {
            int i = pares[k].first;
            int j = pares[k].second;

            // Generamos el vecino
            tSolution<double> vecino;
            bool valido = generarVecino(sol, vecino, i, j, ratio, lo, hi);

            // Si el vecino no es válido, pasamos al siguiente
            if (!valido) continue;

            // Evaluamos el vecino
            double fitness_vecino = problema.fitness(vecino);
            evaluaciones++;
            evals_bl++;

            // Si mejora, lo aceptamos y dejamos de recorrer el vecindario
            if (fitness_vecino > fitness_sol) {
                sol = vecino;
                fitness_sol = fitness_vecino;
                mejora = true;
            }
            if (mejora) break;
        }

        // Si no se ha encontrado ninguna mejora, se termina la búsqueda (óptimo local)
        if (!mejora) break;
    }
}

// Decide a qué cromosomas aplicar la BL suave según el tipo de AM.
void MemeticAlgorithm::aplicarBL(vector<Individuo>& poblacion, ProblemDouble& problema, int& evaluaciones) {

    if (tipo_am == "ALL") {

        // AM-All: aplica la BL a todos los cromosomas de la población
        for (int i = 0; i < (int)poblacion.size() && evaluaciones < evaluaciones_max; i++) {
            busquedaLocalSuave(poblacion[i].sol, poblacion[i].fitness, problema, evaluaciones);
        }
    }  else if (tipo_am == "RAND") {

        // AM-Rand: aplica la BL a cada cromosoma con probabilidad 0.1
        for (int i = 0; i < (int)poblacion.size() && evaluaciones < evaluaciones_max; i++) {
            double p = Random::get<double>(0.0, 1.0);

            if (p <= 0.1) {
                busquedaLocalSuave(poblacion[i].sol, poblacion[i].fitness, problema, evaluaciones);
            }
        }
    } else if (tipo_am == "BEST") {

        // AM-Best: ordena la población y aplica la BL al 10% de mejores cromosomas
        vector<int> indices(poblacion.size());
        for (int i = 0; i < (int)poblacion.size(); i++) {
            indices[i] = i;
        }

        // Ordenamos los índices según el fitness
        sort(indices.begin(), indices.end(),[&](int a, int b) {
                 return poblacion[a].fitness > poblacion[b].fitness;
             });

        // Aplicamos BL sobre los 0,1 ⋅ 𝑁 mejores cromosomas de la población actual
        int n = (int)(0.1 * poblacion.size());

        for (int k = 0; k < n && evaluaciones < evaluaciones_max; k++) {
            int i = indices[k];
            busquedaLocalSuave(poblacion[i].sol, poblacion[i].fitness, problema, evaluaciones);
        }

    } else if (tipo_am == "BEST1") {

        // EXTRA -> AM-BEST1: aplica la BL únicamente al mejor cromosoma de la población
        int mejor_individuo = mejorIndividuo(poblacion);
        if (evaluaciones < evaluaciones_max) {
            busquedaLocalSuave(poblacion[mejor_individuo].sol,  poblacion[mejor_individuo].fitness, problema, evaluaciones);
        }
    }
}

// Algoritmo Memético: AGG con BL aplicada cada 10 generaciones
ResultMH<double> MemeticAlgorithm::ejecutarAM(Problem<double>& problema) {

    Portfolio& portfolio = dynamic_cast<Portfolio&>(problema); // Necesito hacerlo para el fix
    int evaluaciones = 0; // Cuenta las evaluaciones de fitness
    int generacion = 0;  // Cuenta las generaciones para aplicar la BL cada 10

    // 1) Creamos la población inicial: inicializamos una población de 100 individuos
    vector<Individuo> poblacion = inicializarPoblacion(problema, evaluaciones);

    // Calculamos el número de cruces en el modelo generacional
    int numero_cruces = (int)(prob_cruce * tamaño_poblacion / 2); 

    // Guardo el mejor fitness encontrado hasta el momento
    double mejor_solucion = poblacion[mejorIndividuo(poblacion)].fitness;

    // Repetimos hasta alcanzar el máximo de evaluaciones, incluyendo las evaluaciones de la BL
    while (evaluaciones < evaluaciones_max) {

        generacion++;  // Avanzo una generación del AGG base

        // 2) Selección:  se aplicarán tantos torneos como individuos existan en la población genética, y se añaden a la nueva población
        vector<Individuo> padres;
        for (int i = 0; i < tamaño_poblacion; i++) {
            int ganador = seleccionarPorTorneo(poblacion);
            padres.push_back(poblacion[ganador]);
        }

        // 3) Cruce: las primeras numero_cruces parejas se cruzan, el resto se copia.
        vector<Individuo> hijos = padres;

        // Como el mecanismo de selección ya tiene una componente aleatoria, se aplica siempre un emparejamiento fijo: el primero con el segundo, el tercero con el cuarto, etc.
        for (int p = 0; p < numero_cruces * 2; p += 2) {

            tSolution<double> hijo1, hijo2;

            if (tipo_cruce == "ARIT") cruceAritmetico(padres[p].sol, padres[p + 1].sol, hijo1, hijo2);
            else cruceBLX(padres[p].sol, padres[p + 1].sol, hijo1, hijo2, alpha_BLX);

            // Los operadores de cruce pueden violar las restricciones, así que reparamos los hijos.
            portfolio.fix(hijo1);
            portfolio.fix(hijo2);

            hijos[p].sol = hijo1;
            hijos[p + 1].sol = hijo2;
        }

        // 4) Mutación: probabilidad 0.1 por individuo 
        for (int i = 0; i < tamaño_poblacion; i++) {
            if (Random::get<double>(0.0, 1.0) <= prob_mutacion) mutar(hijos[i].sol, problema);
        }

        // 5) Evaluación de los hijos
        for (int i = 0; i < tamaño_poblacion && evaluaciones < evaluaciones_max; i++) {
            // Evalúo el fitness de cada hijo y aumento el número de evaluaciones
            hijos[i].fitness = problema.fitness(hijos[i].sol);
            evaluaciones++;

            // Actualizo el mejor fitness encontrado
            if (hijos[i].fitness > mejor_solucion) {
                mejor_solucion = hijos[i].fitness;
            }

        }

        // 6) Reemplazo con elitismo: si el mejor de la población anterior es mejor que el mejor de los hijos, sustituye al peor de los hijos.
        int mejor_antiguo = mejorIndividuo(poblacion);
        int mejor_nuevo = mejorIndividuo(hijos);

        if (poblacion[mejor_antiguo].fitness > hijos[mejor_nuevo].fitness) {
            int peor_nuevo = peorIndividuo(hijos);
            hijos[peor_nuevo] = poblacion[mejor_antiguo];
        }

        // Actualizo la población
        poblacion = hijos;

        // 7) Cada 10 generaciones, aplico la BL suave según la variante del AM
        if ((generacion % 10 == 0) && (evaluaciones < evaluaciones_max)) {
            aplicarBL(poblacion, problema, evaluaciones);
            mejor_solucion = poblacion[mejorIndividuo(poblacion)].fitness;
        }
    }

    // Devuelvo la mejor solución encontrada
    int mejor = mejorIndividuo(poblacion);

    return ResultMHDouble(poblacion[mejor].sol, poblacion[mejor].fitness, evaluaciones);
}
