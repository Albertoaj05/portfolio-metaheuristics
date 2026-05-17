#!/usr/bin/env python3

# Dibuja la sensibilidad de AGG-BLX al cambiar el tamano del torneo.

import csv
import os

import matplotlib.pyplot as plt


DATASETS = [
    "Ibex 35",
    "S&P 100",
    "S&P 500",
]

# Valores de k probados en el experimento extra.
VALORES_K = [
    2,
    3,
    5,
    7,
]

COLORES = [
    "#d7ecd3",
    "#9fd49a",
    "#62b46f",
    "#2f8f57",
]

RUTA_DATOS = "resultados/sensibilidad_torneo.csv"
RUTA_SALIDA = "graficas/torneo_k/torneo_k.png"


def leer_datos():
    # Lee el fitness medio obtenido para cada dataset y valor de k.
    datos = {}

    with open(RUTA_DATOS) as fichero:
        lector = csv.DictReader(fichero)

        for fila in lector:
            dataset = fila["dataset"]
            k = int(fila["k"])
            fitness_medio = float(fila["fitness_medio"])

            if dataset not in datos:
                datos[dataset] = {}

            datos[dataset][k] = fitness_medio

    return datos


def calcular_limites_y(valores):
    # Ajusta el eje Y para comparar mejor las barras.
    minimo = min(valores)
    maximo = max(valores)
    margen = max(0.05, (maximo - minimo) * 0.25)

    return minimo - margen, maximo + margen


def marcar_mejor_barra(barras, valores):
    # Resalta el valor de k con mejor fitness medio.
    indice_mejor = valores.index(max(valores))
    barras[indice_mejor].set_edgecolor("black")
    barras[indice_mejor].set_linewidth(2)


def dibujar_dataset(ax, nombre_dataset, datos):
    ax.set_title(nombre_dataset)

    if nombre_dataset not in datos:
        return

    etiquetas = [f"k={k}" for k in VALORES_K]
    valores = [datos[nombre_dataset].get(k, 0.0) for k in VALORES_K]

    # Dibuja una barra por cada valor de k.
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
    if not os.path.exists(RUTA_DATOS):
        print(f"AVISO: no existe {RUTA_DATOS}")
        return

    datos = leer_datos()

    # Una subgrafica por dataset.
    figura, axes = plt.subplots(1, 3, figsize=(13, 4.5))

    for ax, nombre_dataset in zip(axes, DATASETS):
        dibujar_dataset(ax, nombre_dataset, datos)

    figura.suptitle("Sensibilidad de k en torneo (AGG-BLX)", fontsize=12)

    os.makedirs(os.path.dirname(RUTA_SALIDA), exist_ok=True)
    plt.tight_layout()
    # Guarda la grafica conjunta.
    plt.savefig(RUTA_SALIDA, dpi=300)
    plt.close()

    print(f"Generado: {RUTA_SALIDA}")


if __name__ == "__main__":
    main()
