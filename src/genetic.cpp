#include "../inc/genetic.h"
#include "../inc/portfolio.h"
#include "../common/random.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace std;

// Constructor de GeneticAlgorithm, inicializa el modelo, el tipo de cruce y los parámetros comunes.
GeneticAlgorithm::GeneticAlgorithm(const string& modelo, const string& cruce) : MHDouble() {

    tipo_modelo = modelo;       // Tipo de modelo: "AGG" (Generacional con elitismo) o "AGE" (Estacionario)
    tipo_cruce = cruce;         // Tipo de cruce: "ARIT", "BLX" o "UNIF" (extra)
    tamaño_poblacion = 100;     // Tamaño de 100 cromosomas
    evaluaciones_max = 10000;   // Tamaño máximo de evaluaciones
    k_torneo = 3;               // Torneo con 3 individuos
    prob_cruce_AGG = 0.8;       // Probabilidad de cruce por pareja en AGG
    prob_mutacion = 0.1;        // Probabilidad de mutación por individuo
    alpha_BLX = 0.3;            // Parámetro α del cruce BLX-α 
}

// Método que inicializa el número de evaluaciones elegido y ejecuta el modelo genético seleccionado.
ResultMH<double> GeneticAlgorithm::optimize(Problem<double>& problema, int evaluaciones) {

    evaluaciones_max = evaluaciones;

    if (tipo_modelo == "AGG") return ejecutarAGG(problema);
    else return ejecutarAGE(problema);

}

// Método que genera la población inicial con soluciones aleatorias del problema.
vector<GeneticAlgorithm::Individuo> GeneticAlgorithm::inicializarPoblacion(ProblemDouble& problema, int& evaluaciones) {

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
int GeneticAlgorithm::seleccionarPorTorneo(const vector<Individuo>& poblacion) {

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
int GeneticAlgorithm::mejorIndividuo(const vector<Individuo>& poblacion) const {

    int mejor_individuo = -1;

    for (int i = 0; i < (int)poblacion.size(); i++) {
        if ((mejor_individuo == -1) || (poblacion[i].fitness > poblacion[mejor_individuo].fitness)) {
            mejor_individuo = i;
        }
    }

    return mejor_individuo;
}

// Método que devuelve el índice del peor individuo, es decir, el de menor fitness
int GeneticAlgorithm::peorIndividuo(const vector<Individuo>& poblacion) const {

    int peor_individuo = -1;

    for (int i = 0; i < (int)poblacion.size(); i++) {
        if (peor_individuo == -1 || poblacion[i].fitness < poblacion[peor_individuo].fitness) {
            peor_individuo = i;
        }
    }

    return peor_individuo;
}

// Método que devuelve el índice del segundo peor, excluimos al peor saltándolo.
int GeneticAlgorithm::segundoPeorIndividuo(const vector<Individuo>& poblacion, int peor) const {

    int segundo_peor= -1;

    for (int i = 0; i < (int)poblacion.size(); i++) {
        if (i == peor) continue; // Si i == peor, lo saltamos, buscamos el siguiente peor

        if (segundo_peor == -1 || poblacion[i].fitness < poblacion[segundo_peor].fitness) {
            segundo_peor = i;
        }
    }

    return segundo_peor;
}

// Método que implementa el Cruce Aritmético
void GeneticAlgorithm::cruceAritmetico(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2) {

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
void GeneticAlgorithm::cruceBLX(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2, double alpha) {

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

// EXTRA: Cruce uniforme, cada gen se toma aleatoriamente de uno de los dos padres.
void GeneticAlgorithm::cruceUniforme(const tSolution<double>& padre1, const tSolution<double>& padre2, tSolution<double>& hijo1, tSolution<double>& hijo2) {

    size_t n = padre1.size();

    // Reserva el tamaño de los hijos antes de asignar cada gen
    hijo1.resize(n);
    hijo2.resize(n);

    for (size_t i = 0; i < n; i++) {
        // Con probabilidad 0.5, hijo1 toma el gen de padre1; si no, lo toma de padre2
        if (Random::get<double>(0.0, 1.0) < 0.5) {
            hijo1[i] = padre1[i];
            hijo2[i] = padre2[i];
        } else {
            hijo1[i] = padre2[i];
            hijo2[i] = padre1[i];
        }
    }
}

// Operador de mutación por intercambio
void GeneticAlgorithm::mutar(tSolution<double>& sol, ProblemDouble& problema) {
    
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

// Algoritmo Genético Generacional con elitismo, designado como AGG.
ResultMHDouble GeneticAlgorithm::ejecutarAGG(ProblemDouble& problema) {

    Portfolio& portfolio = dynamic_cast<Portfolio&>(problema); // Necesito hacerlo para el fix
    int evaluaciones = 0; // Cuenta las evaluaciones de fitness

    // 1) Creamos la población inicial: inicializamos una población de 100 individuos
    vector<Individuo> poblacion = inicializarPoblacion(problema, evaluaciones);

    // Calculamos el número de cruces en el modelo generacional
    int numero_cruces = (int)(prob_cruce_AGG * tamaño_poblacion / 2); 

    // Guardo el mejor fitness encontrado hasta el momento
    double mejor_solucion = poblacion[mejorIndividuo(poblacion)].fitness;

    // Repetimos generaciones hasta alcanzar el máximo de evaluaciones
    while (evaluaciones < evaluaciones_max) {

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
            else if (tipo_cruce == "UNIF") cruceUniforme(padres[p].sol, padres[p + 1].sol, hijo1, hijo2);
            else cruceBLX(padres[p].sol, padres[p + 1].sol, hijo1, hijo2, alpha_BLX);

            // Los operadores de cruce pueden violar las restricciones, así que reparamos los hijos.
            portfolio.fix(hijo1);
            portfolio.fix(hijo2);

            // Asignamos los hijos al vector de individuos
            hijos[p].sol = hijo1;
            hijos[p + 1].sol = hijo2;
        }

        // 4) Mutación: probabilidad 0.1 por individuo 
        for (int i = 0; i < tamaño_poblacion; i++) {
            if (Random::get<double>(0.0, 1.0) <= prob_mutacion) mutar(hijos[i].sol, problema);
        }

        // 5) Evaluación de los hijos
        for (int i = 0; i < tamaño_poblacion; i++) {
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
    }

    // Devuelvo la mejor solución encontrada
    int mejor = mejorIndividuo(poblacion);

    return ResultMHDouble(poblacion[mejor].sol, poblacion[mejor].fitness, evaluaciones);
}


// Algoritmo Genético Estacionario, designado como AGE.
ResultMHDouble GeneticAlgorithm::ejecutarAGE(ProblemDouble& problema) {


    Portfolio& portfolio = dynamic_cast<Portfolio&>(problema); // Necesito hacerlo para el fix
    int evaluaciones = 0; // Cuenta las evaluaciones de fitness

    // 1) Creamos la población inicial: inicializamos una población de 100 individuos
    vector<Individuo> poblacion = inicializarPoblacion(problema, evaluaciones);

    // Guardo el mejor fitness encontrado hasta el momento
    double mejor_solucion = poblacion[mejorIndividuo(poblacion)].fitness;

    // AGE genera 2 hijos por iteración.
    while (evaluaciones < evaluaciones_max) {

        // 2) Selección: elegir dos padres por torneo.
        int p1 = seleccionarPorTorneo(poblacion);
        int p2 = seleccionarPorTorneo(poblacion);

        // 3) Cruce: realizamos el cruce siempre
        tSolution<double> hijo1, hijo2;

        if (tipo_cruce == "ARIT") cruceAritmetico(poblacion[p1].sol, poblacion[p2].sol, hijo1, hijo2);
        else if (tipo_cruce == "UNIF") cruceUniforme(poblacion[p1].sol, poblacion[p2].sol, hijo1, hijo2);
        else cruceBLX(poblacion[p1].sol, poblacion[p2].sol, hijo1, hijo2, alpha_BLX);

        // Los operadores de cruce pueden violar las restricciones, así que reparamos los hijos.
        portfolio.fix(hijo1);
        portfolio.fix(hijo2);

        // 4) Mutación: probabilidad 0.1 por individuo 
        if (Random::get<double>(0.0, 1.0) <= prob_mutacion) mutar(hijo1, problema);
        if (Random::get<double>(0.0, 1.0) <= prob_mutacion) mutar(hijo2, problema);

        // 5) Evaluación de los hijos
        Individuo h1, h2;

        h1.sol = hijo1;
        h2.sol = hijo2;

        h1.fitness = problema.fitness(h1.sol);
        h2.fitness = problema.fitness(h2.sol);

        evaluaciones += 2;

        // Actualizo el mejor fitness encontrado
        if (h1.fitness > mejor_solucion) mejor_solucion = h1.fitness;
        if (h2.fitness > mejor_solucion) mejor_solucion = h2.fitness;

        // 6) Reemplazo: de los 2 peores actuales y los 2 hijos, sobreviven los 2 mejores.
        int peor = peorIndividuo(poblacion);
        int segundo_peor = segundoPeorIndividuo(poblacion, peor);

        vector<Individuo> candidatos = { poblacion[peor], poblacion[segundo_peor], h1, h2};

        // Ordena los candidatos de mejor a peor fitness
        sort(candidatos.begin(), candidatos.end(),
            [](const Individuo& a, const Individuo& b) {
                return a.fitness > b.fitness;
            });

        poblacion[peor] = candidatos[0];
        poblacion[segundo_peor] = candidatos[1];
    }

    // Devuelve la mejor solución encontrada
    int mejor = mejorIndividuo(poblacion);

    return ResultMHDouble(poblacion[mejor].sol, poblacion[mejor].fitness, evaluaciones);
}
