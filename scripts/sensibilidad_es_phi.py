#!/usr/bin/env python3

# Dibuja la sensibilidad del ES respecto al parametro phi.

import csv
import os

import matplotlib.pyplot as plt


DATASETS = ["Ibex 35", "S&P 100", "S&P 500"]
VALORES_PHI = [0.1, 0.3, 0.5]
RUTA_DATOS = "resultados/sensibilidad_es_phi.csv"
RUTA_SALIDA = "graficas/sensibilidad_es/phi.png"


def leer_datos():
    # Lee el fitness medio obtenido con cada valor de phi.
    datos = {}

    if not os.path.exists(RUTA_DATOS):
        print(f"AVISO: no existe {RUTA_DATOS}")
        return datos

    with open(RUTA_DATOS) as fichero:
        lector = csv.DictReader(fichero)

        for fila in lector:
            dataset = fila["dataset"]
            phi = float(fila["phi"])
            fitness = float(fila["fitness_2015_2024"])
            datos.setdefault(dataset, {})[phi] = fitness

    return datos


def limites(valores):
    # Ajusta el eje Y para que las diferencias pequenas se vean mejor.
    minimo = min(valores)
    maximo = max(valores)
    margen = max(0.02, (maximo - minimo) * 0.3)
    return minimo - margen, maximo + margen


def main():
    datos = leer_datos()
    if not datos:
        return

    fig, axes = plt.subplots(1, 3, figsize=(13, 4.4))

    for ax, dataset in zip(axes, DATASETS):
        valores = [datos.get(dataset, {}).get(phi, 0.0) for phi in VALORES_PHI]
        etiquetas = [f"phi={phi}" for phi in VALORES_PHI]

        barras = ax.bar(etiquetas, valores, color=["#B0B0B0", "#55A868", "#C44E52"])
        ax.set_title(dataset)
        ax.set_ylabel("Fitness 2015-2024")
        ax.grid(True, axis="y", alpha=0.3)
        ax.set_ylim(*limites(valores))

        mejor = valores.index(max(valores))
        barras[mejor].set_edgecolor("black")
        barras[mejor].set_linewidth(2)

        for barra, valor in zip(barras, valores):
            ax.text(barra.get_x() + barra.get_width() / 2, valor, f"{valor:.3f}", ha="center", va="bottom", fontsize=8)

    fig.suptitle("Sensibilidad de ES respecto a phi")
    os.makedirs(os.path.dirname(RUTA_SALIDA), exist_ok=True)
    plt.tight_layout()
    plt.savefig(RUTA_SALIDA, dpi=300)
    plt.close()


if __name__ == "__main__":
    main()
