#include "../inc/portfolio.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

using namespace std;



// Constructor de la clase Portfolio
Portfolio::Portfolio(const string& filename, double lo, double hi, double lambda){

    this->lo = lo;           // peso mínimo permitido 
    this->hi = hi;           // peso máximo permitido
    this->lambda = lambda;   // penalización del riesgo

    this->n = 0; // número de empresas (columnas)
    this->T = 0; // número de días (filas)

    // Abrimos el fichero CSV
    ifstream file(filename);

    // Si no se puede abrir, lanzamos error
    if (!file.is_open()) {
        throw runtime_error("Error abriendo fichero: " + filename);
    }

    string line;

    // Leemos la cabecera para saber n
    if (getline(file, line)) {
        stringstream ss(line);
        string cell;

        // Ignoramos la primera columna, ya que es la fecha
        getline(ss, cell, ',');

        // Contamos el resto de columnas, ya que estas si son empresas
        while (getline(ss, cell, ',')) {
            n++;
        }
    }

    // Leemos los datos que corresponden al mu
    while (getline(file, line)) {
        if (line.empty()) continue; // ignoramos líneas vacías

        stringstream ss(line);
        string cell;
        vector<double> row;

        // Leemos la fecha (primera columna)
        string fecha;
        getline(ss, fecha, ',');

        // Nos quedamos solo con datos hasta 2024-12-31
        if (fecha > "2024-12-31") {
            continue;
        }

        // Leemos los valores de cada empresa
        while (getline(ss, cell, ',')) {
            row.push_back(stod(cell)); // convertimos a double
        }

        // Comprobamos que la fila tiene el tamaño correcto
        if (row.size() != n) {
            throw runtime_error("Fila con número incorrecto de columnas");
        }

        // Guardamos la fila
        mu.push_back(row);
    }

    file.close();

    // Número total de días leídos
    T = mu.size();

    // Cálculo de la media por acción y de la parte interna de la fórmula del beneficio acumulado,
    // ya que falta por multiplicar por w(i)
    media_por_accion = vector<double>(n, 0.0);
    parte_interna_beneficio = vector<double>(n, 0.0);

    for (size_t j = 0; j < n; j++) {

        double suma_mu = 0.0;
        double suma_log = 0.0;

        for (size_t i = 0; i < T; i++) {
            suma_mu += mu[i][j];                // media normal
            suma_log += log(1.0 + mu[i][j]);    // acumulamos el término del beneficio logarítmico
        }

        // Calculamos la media por acción
        media_por_accion[j] = suma_mu / T;

        // Asignamos la suma_log a esta variable para usarla más adelante
        parte_interna_beneficio[j] = suma_log;
    }

    // Calculamos la matriz de covarianza entre las distintas empresas
    cov = vector<vector<double>>(n, vector<double>(n, 0.0));

    // Calculamos covarianza entre cada par de empresas
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {

            double sum = 0.0;

            for (size_t k = 0; k < T; k++) {
                // Fórmula de la covarianza
                sum += (mu[k][i] - media_por_accion[i]) *  (mu[k][j] - media_por_accion[j]);
            }

            // Dividimos entre T 
            cov[i][j] = sum / T;
        }
    }
}



// Función que comprueba si una solucion es válida, es decir, que la suma de sus pesos sea ≈ 1
// y que los pesos estén dentro de un rango permitido
bool Portfolio::isValid(const tSolution<double>& solucion) {

    const double EPS = 1e-8; // Margen de error fijado en el guión
    double suma_pesos = 0.0;

    // Comprueba adicionalmente si una solución tiene el tamaño correcto
    if (solucion.size() != n) {
        return false;
    }

    for (size_t i = 0; i < solucion.size(); i++) {
        double peso = solucion[i];

        // Si el valor es muy pequeño, lo consideramos 0 (evita errores numéricos)
        if (fabs(peso) < EPS) {
            peso = 0.0;
        }

        // Cada valor tiene que estar en un rango limitado: (wi == 0) ∨ (lo ≤ wi ≤ hi), ∀i ∈ 1..N
        if (peso != 0.0 && (peso < lo - EPS || peso > hi + EPS)) {
            return false;
        }

        suma_pesos += peso;
    }

    // La suma debe ser aproximadamente 1, permitiendo el margen de error
    return fabs(suma_pesos - 1.0) < EPS;
}


// Función que repara una solución
void Portfolio::fix(tSolution<double>& solucion) {

    const double EPS = 1e-8; // Margen de error fijado en el guión

    // 1) Normalizamos si la suma no es 1
    double suma = 0.0;
    for (size_t i = 0; i < solucion.size(); i++) {
        suma += solucion[i];
    }

    if (suma > 0.0) {
        for (size_t i = 0; i < solucion.size(); i++) {
            solucion[i] /= suma;
        }
    }

    // 2) Asegurar que cada peso esté en el rango:
    //    wi <- 0 si wi < lo, y wi <- hi si wi > hi
    for (size_t i = 0; i < solucion.size(); i++) {
        if (solucion[i] < lo) {
            solucion[i] = 0.0;
        }
        else if (solucion[i] > hi) {
            solucion[i] = hi;
        }
    }

    // 3) Recalcular la suma tras forzar el rango
    suma = 0.0;
    for (size_t i = 0; i < solucion.size(); i++) {
        suma += solucion[i];
    }

    // 4) Si la suma es menor que 1, repartir la diferencia entre pesos > 0 y < hi
    while (suma < 1.0 - EPS) {

        // Calculamos la diferencia que falta
        double dif = 1.0 - suma;

        // Buscamos los pesos que pueden crecer
        vector<size_t> incrementables;
        for (size_t i = 0; i < solucion.size(); i++) {
            if (solucion[i] > 0.0 && solucion[i] < hi) {
                incrementables.push_back(i);
            }
        }

        // Dividimos la diferencia entre los pesos incrementables
        if (!incrementables.empty()) {
            double incremento = dif / incrementables.size();

            for (size_t k = 0; k < incrementables.size(); k++) {
                size_t i = incrementables[k];

                // Si se puede subir el incremento entero se sube
                if (solucion[i] + incremento <= hi) {
                    solucion[i] += incremento;
                // Si no se puede subir el incremento, lo dejo justo en hi
                } else {
                    solucion[i] = hi;
                }
            }
        }
        else {
            // Si no es suficiente, usar pesos iguales a 0 elegidos aleatoriamente
            vector<size_t> ceros;
            for (size_t i = 0; i < solucion.size(); i++) {
                if (solucion[i] == 0.0) {
                    ceros.push_back(i);
                }
            }

            // Si no hay ningún cero, ya no puedo activar ningún peso de nuevo
            if (ceros.empty()) {
                break;
            }

            // Mezclamos aleatoriamente para que no se elija siempre el mismo
            shuffle(ceros.begin(), ceros.end(), Random::get_engine());

            bool cambiado = false;
            for (size_t k = 0; k < ceros.size(); k++) {
                size_t i = ceros[k];

                // Si la diferencia cabe en el rango, le asignamos dif
                if (dif >= lo) {
                    if (dif <= hi) {
                        solucion[i] = dif;
                    // Si no, le asigno hi, al igual que antes
                    } else {
                        solucion[i] = hi;
                    }
                    cambiado = true;
                    break;
                }
            }

            // Si no se ha podido cambiar nada, se sale
            if (!cambiado) {
                break;
            }
        }

        // Recalculamos la suma tras los cambios
        suma = 0.0;
        for (size_t i = 0; i < solucion.size(); i++) {
            suma += solucion[i];
        }
    }

    // 5) Si la suma es mayor que 1, restar la diferencia a pesos > lo
    while (suma > 1.0 + EPS) {

        // Calculamos el exceso
        double exceso = suma - 1.0;

        // Buscamos pesos que sean reducibles
        vector<size_t> reducibles;
        for (size_t i = 0; i < solucion.size(); i++) {
            if (solucion[i] > lo) {
                reducibles.push_back(i);
            }
        }

        // Si no hay reducibles, se para
        if (reducibles.empty()) {
            break;
        }

        double decremento = exceso / reducibles.size();

        // Se reduce el exceso, respetando siempre los rangos
        for (size_t k = 0; k < reducibles.size(); k++) {
            size_t i = reducibles[k];

            if (solucion[i] - decremento >= lo) {
                solucion[i] -= decremento;
            } else {
                solucion[i] = lo;
            }
        }

        // Volvemos a recalcular la suma
        suma = 0.0;
        for (size_t i = 0; i < solucion.size(); i++) {
            suma += solucion[i];
        }
    }
}



// Función que crea una solución aleatoria
tSolution<double> Portfolio::createSolution() {
    tSolution<double> solucion(n, 0.0);

    // Número de ceros entre 1 y N-1
    size_t n_ceros = Random::get(1, (int)n -1);

    // Creamos índices y los barajamos
    vector<size_t> indices(n);
    for(size_t i = 0; i < n; i++){
        indices[i] = i;
    }
    shuffle(indices.begin(), indices.end(), Random::get_engine());

    // Los n_ceros primeros se quedan a 0, tal cual están
    // Al resto les asignamos valores aleatorios dentro de los rangos permitidos
    for(size_t i = n_ceros; i < n; i++){
        size_t k = indices[i];
        solucion[k] = Random::get<double>(lo, hi);
    }

    // Normalizamos
    double suma = 0.0;
    for(size_t i = 0; i < solucion.size(); i++){
        suma += solucion[i];
    }
    
    // Controlamos que no dividimos entre 0
    if (suma > 0.0){
        for(size_t i = 0; i < solucion.size(); i++){
            solucion[i] = solucion[i] / suma;
        }
    }
    // Reparamos
    fix(solucion);

    return solucion;
}



// Función que calcula el fitness de una solución
tFitness Portfolio::fitness(const tSolution<double>& w) {

    double beneficio_total = 0.0;
    double riesgo = 0.0;

    for (size_t i = 0; i < n; i++) {
         beneficio_total += w[i] * parte_interna_beneficio[i];
    }

    for (size_t i = 0; i < n; i++) {
        if (w[i] == 0.0) continue; 
        riesgo += w[i] * w[i] * cov[i][i];

        for (size_t j = i + 1; j < n; j++) {
            riesgo += 2.0 * w[i] * cov[i][j] * w[j];
        }
    }

    riesgo = sqrt(riesgo);

    return beneficio_total - lambda * riesgo;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////// DEFINIMOS LOS GETTERS //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Getter que devuelve la media por acción
const vector<double>& Portfolio::getMediaPorAccion() const {
return media_por_accion;
}

// Getter que devuelve la parte interna del beneficio acumulado (∑ log(1+mu))
const vector<double>& Portfolio::getBeneficio() const {
return parte_interna_beneficio;
}

// Getter que devuelve la matriz de covarianza entre empresas
const vector<vector<double>>& Portfolio::getCov() const {
    return cov;
}

// Getter que devuelve el número de empresas n
size_t Portfolio::getSolutionSize() {
    return n;
}

// Getter que devuelve el número de días
size_t Portfolio::getT() const {
    return T;
}

// Getter que devuelve la matriz mu con los retornos diarios
const vector<vector<double>>& Portfolio::getMu() const {
    return mu;
}

// Getter que devuelve el rango permitido para los pesos de cada empresa
pair<double, double> Portfolio::getSolutionDomainRange() {
    return {0.0, hi};
}

// Getter que devuelve el valor de lambda (penalización del riesgo)
double Portfolio::getLambda() const {
    return lambda;
}

// Getter que devuelve el valor máximo permitido para cada peso
double Portfolio::getHi() const {
    return hi;
}

// Getter que devuelve el valor mínimo permitido para cada peso
double Portfolio::getLo() const {
    return lo;
}
