#!/usr/bin/env python3

# Genera graficas de barras con el tiempo medio de cada algoritmo.

import csv
import os

import matplotlib.pyplot as plt


RUTA_DATOS = "resultados/resultados_medias.csv"

# Datasets y nombres de salida de sus graficas.
DATASETS = [
    ("Ibex 35", "ibex_35"),
    ("S&P 100", "syp_100"),
    ("S&P 500", "syp_500"),
]

ORDEN_ALGORITMOS = [
    "Greedy",
    "Random",
    "BL",
    "AGG-Arit",
    "AGG-BLX",
    "AGE-Arit",
    "AGE-BLX",
    "AM-All",
    "AM-Rand",
    "AM-Best",
]

COLORES = {
    "Greedy": "#B0B0B0",
    "Random": "#9E9E9E",
    "BL": "#4C72B0",
    "AGG-Arit": "#55A868",
    "AGG-BLX": "#2E8B57",
    "AGG-Unif": "#1E5631",
    "AGE-Arit": "#C44E52",
    "AGE-BLX": "#8B0000",
    "AGE-Unif": "#5C0000",
    "AM-All": "#8172B3",
    "AM-Rand": "#9370DB",
    "AM-Best": "#6A5ACD",
    "AM-Best1": "#483D8B",
}


def leer_tiempos(ruta_csv):
    # Lee el tiempo medio de cada algoritmo por dataset.
    datos = {}

    with open(ruta_csv) as fichero:
        lector = csv.DictReader(fichero)

        for fila in lector:
            dataset = fila["dataset"]
            algoritmo = fila["algoritmo"]
            tiempo = float(fila["tiempo_seg"])

            if dataset not in datos:
                datos[dataset] = {}

            datos[dataset][algoritmo] = tiempo

    return datos


def crear_directorio_salida():
    os.makedirs("graficas/tiempos", exist_ok=True)


def anadir_etiquetas(barras, valores):
    # Escribe el valor numerico encima de cada barra.
    for barra, valor in zip(barras, valores):
        plt.text(
            barra.get_x() + barra.get_width() / 2,
            valor,
            f"{valor:.3f}",
            ha="center",
            va="bottom",
            fontsize=8,
        )


def guardar_grafica(nombre_dataset, datos_dataset, sufijo):
    # Mantiene el mismo orden de algoritmos en todos los datasets.
    valores = [datos_dataset.get(algoritmo, 0.0) for algoritmo in ORDEN_ALGORITMOS]
    colores = [COLORES[algoritmo] for algoritmo in ORDEN_ALGORITMOS]

    plt.figure(figsize=(13, 5.5))
    barras = plt.bar(ORDEN_ALGORITMOS, valores, color=colores)

    plt.title(f"Tiempo medio - {nombre_dataset}")
    plt.xlabel("Algoritmo")
    plt.ylabel("Tiempo (s)")
    plt.xticks(rotation=45, ha="right")
    plt.grid(True, axis="y", alpha=0.3)

    anadir_etiquetas(barras, valores)

    plt.tight_layout()

    crear_directorio_salida()
    ruta_salida = f"graficas/tiempos/{sufijo}.png"
    # Guarda la grafica de tiempos del dataset.
    plt.savefig(ruta_salida, dpi=140)
    plt.close()

    print(f"Generado: {ruta_salida}")


def main():
    if not os.path.exists(RUTA_DATOS):
        print(f"AVISO: no existe {RUTA_DATOS}")
        return

    datos = leer_tiempos(RUTA_DATOS)

    for nombre_dataset, sufijo in DATASETS:
        if nombre_dataset in datos:
            guardar_grafica(nombre_dataset, datos[nombre_dataset], sufijo)


if __name__ == "__main__":
    main()
