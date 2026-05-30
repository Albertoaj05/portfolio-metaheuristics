#!/usr/bin/env python3

# Dibuja las curvas de mejor fitness acumulado durante la ejecucion.

import csv
import os

import matplotlib.pyplot as plt


DATASETS = [
    ("ibex_35", "Ibex 35"),
    ("syp_100", "S&P 100"),
    ("syp_500", "S&P 500"),
]

# Algoritmos que se muestran en cada curva de convergencia.
ORDEN_ALGORITMOS = [
    "Random",
    "BL",
    "AM-Best",
    "BMB",
    "ILS-BL",
    "ES",
    "ILS-ES",
]

COLORES = {
    "Random": "#7f7f7f",
    "BL": "#1f77b4",
    "AM-Best": "#9467bd",
    "BMB": "#2ca02c",
    "ILS-BL": "#ff7f0e",
    "ES": "#d62728",
    "ILS-ES": "#17becf",
}


def leer_convergencia(ruta_csv):
    # Lee evaluacion y mejor fitness acumulado de cada algoritmo.
    datos = {}

    with open(ruta_csv) as fichero:
        lector = csv.DictReader(fichero)

        for fila in lector:
            algoritmo = fila["algoritmo"]
            evaluaciones = int(fila["evaluaciones"])
            fitness = float(fila["fitness"])

            if algoritmo not in datos:
                datos[algoritmo] = {
                    "evaluaciones": [],
                    "fitness": [],
                }

            datos[algoritmo]["evaluaciones"].append(evaluaciones)
            datos[algoritmo]["fitness"].append(fitness)

    return datos


def crear_directorio_salida():
    os.makedirs("graficas/convergencia", exist_ok=True)


def dibujar_algoritmo(algoritmo, datos):
    if algoritmo not in datos:
        return

    # Cada linea representa la evolucion del mejor fitness encontrado.
    plt.plot(
        datos[algoritmo]["evaluaciones"],
        datos[algoritmo]["fitness"],
        label=algoritmo,
        color=COLORES[algoritmo],
        linewidth=1.8,
    )


def guardar_grafica(sufijo, nombre_dataset):
    # Crea una grafica de convergencia para el dataset indicado.
    ruta_csv = f"resultados/convergencia_{sufijo}.csv"

    if not os.path.exists(ruta_csv):
        print(f"AVISO: no existe {ruta_csv}")
        return

    datos = leer_convergencia(ruta_csv)

    plt.figure(figsize=(9, 5.5))

    # Dibuja los algoritmos siempre en el mismo orden.
    for algoritmo in ORDEN_ALGORITMOS:
        dibujar_algoritmo(algoritmo, datos)

    plt.xlabel("Evaluaciones")
    plt.ylabel("Mejor fitness")
    plt.title(f"Convergencia en {nombre_dataset}")
    plt.legend(loc="lower right")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    crear_directorio_salida()
    ruta_salida = f"graficas/convergencia/{sufijo}.png"
    # Guarda la curva del dataset actual.
    plt.savefig(ruta_salida, dpi=140)
    plt.close()



def main():
    for sufijo, nombre_dataset in DATASETS:
        guardar_grafica(sufijo, nombre_dataset)


if __name__ == "__main__":
    main()
