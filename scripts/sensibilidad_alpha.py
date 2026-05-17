#!/usr/bin/env python3

# Dibuja la sensibilidad de AGG-BLX ante distintos valores de alpha.

import csv
import os

import matplotlib.pyplot as plt


DATASETS = [
    "Ibex 35",
    "S&P 100",
    "S&P 500",
]

# Valores de alpha probados en el experimento extra.
ALPHAS = [
    0.1,
    0.3,
    0.5,
]

COLORES = [
    "#888888",
    "#2ca02c",
    "#d62728",
]

RUTA_DATOS = "resultados/sensibilidad_alpha.csv"
RUTA_SALIDA = "graficas/sensibilidad_alpha/sensibilidad_alpha.png"


def leer_datos():
    # Lee el fitness medio obtenido para cada dataset y alpha.
    datos = {}

    with open(RUTA_DATOS) as fichero:
        lector = csv.DictReader(fichero)

        for fila in lector:
            dataset = fila["dataset"]
            alpha = float(fila["alpha"])
            fitness_medio = float(fila["fitness_medio"])

            if dataset not in datos:
                datos[dataset] = {}

            datos[dataset][alpha] = fitness_medio

    return datos


def calcular_limites_y(valores):
    # Ajusta el eje Y para que las diferencias se vean mejor.
    minimo = min(valores)
    maximo = max(valores)
    margen = max(0.05, (maximo - minimo) * 0.25)

    return minimo - margen, maximo + margen


def marcar_mejor_barra(barras, valores):
    # Resalta el alpha con mejor fitness medio.
    indice_mejor = valores.index(max(valores))
    barras[indice_mejor].set_edgecolor("black")
    barras[indice_mejor].set_linewidth(2)


def dibujar_dataset(ax, nombre_dataset, datos):
    ax.set_title(nombre_dataset)

    if nombre_dataset not in datos:
        return

    etiquetas = [f"alpha={alpha}" for alpha in ALPHAS]
    valores = [datos[nombre_dataset].get(alpha, 0.0) for alpha in ALPHAS]

    # Dibuja una barra por cada valor de alpha.
    barras = ax.bar(etiquetas, valores, color=COLORES)

    ax.set_ylabel("Fitness medio")
    ax.grid(True, axis="y", alpha=0.3)
    ax.set_ylim(*calcular_limites_y(valores))

    for barra, valor in zip(barras, valores):
        ax.text(
            barra.get_x() + barra.get_width() / 2,
            valor,
            f"{valor:.3f}",
            ha="center",
            va="bottom",
            fontsize=9,
        )

    marcar_mejor_barra(barras, valores)


def main():
    datos = leer_datos()

    # Una subgrafica por dataset.
    figura, axes = plt.subplots(1, 3, figsize=(13, 4.5))

    for ax, nombre_dataset in zip(axes, DATASETS):
        dibujar_dataset(ax, nombre_dataset, datos)

    figura.suptitle("Sensibilidad de alpha en BLX (AGG-BLX)", fontsize=12)

    os.makedirs(os.path.dirname(RUTA_SALIDA), exist_ok=True)
    plt.tight_layout()
    # Guarda la grafica conjunta.
    plt.savefig(RUTA_SALIDA, dpi=300)
    plt.close()

    print(f"Generado: {RUTA_SALIDA}")


if __name__ == "__main__":
    main()
