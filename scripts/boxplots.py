#!/usr/bin/env python3

# Genera los boxplots principales de fitness para cada dataset.

import os

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


DATASETS = [
    ("resultados/boxplot_ibex_35.csv", "Resultados IBEX 35", "graficas/boxplots/ibex_35.png"),
    ("resultados/boxplot_syp_100.csv", "Resultados S&P 100", "graficas/boxplots/syp_100.png"),
    ("resultados/boxplot_syp_500.csv", "Resultados S&P 500", "graficas/boxplots/syp_500.png"),
]

# Nombres y orden usados para que las graficas salgan siempre iguales.
NOMBRES_ALGORITMOS = {
    "Random": "RANDOM",
    "Greedy": "GREEDY",
    "BL": "BL",
    "AGG-Arit": "AGG-ARIT",
    "AGG-BLX": "AGG-BLX",
    "AGE-Arit": "AGE-ARIT",
    "AGE-BLX": "AGE-BLX",
    "AM-All": "AM-ALL",
    "AM-Rand": "AM-RAND",
    "AM-Best": "AM-BEST",
}

ORDEN_ALGORITMOS = [
    "GREEDY",
    "RANDOM",
    "BL",
    "AGG-ARIT",
    "AGG-BLX",
    "AGE-ARIT",
    "AGE-BLX",
    "AM-ALL",
    "AM-RAND",
    "AM-BEST",
]

COLORES = {
    "GREEDY": "#B0B0B0",
    "RANDOM": "#9E9E9E",
    "BL": "#4C72B0",
    "AGG-ARIT": "#55A868",
    "AGG-BLX": "#2E8B57",
    "AGE-ARIT": "#C44E52",
    "AGE-BLX": "#8B0000",
    "AM-ALL": "#8172B3",
    "AM-RAND": "#9370DB",
    "AM-BEST": "#6A5ACD",
}


def preparar_datos(ruta_csv):
    # Carga el CSV y deja solo los algoritmos de la comparacion principal.
    datos = pd.read_csv(ruta_csv)
    datos["algoritmo"] = datos["algoritmo"].replace(NOMBRES_ALGORITMOS)
    datos = datos[datos["algoritmo"].isin(ORDEN_ALGORITMOS)]
    return datos


def crear_directorio_salida(ruta_salida):
    directorio = os.path.dirname(ruta_salida)
    os.makedirs(directorio, exist_ok=True)


def anadir_separadores(ax):
    # Separa visualmente P1, algoritmos geneticos y memeticos.
    ax.axvline(2.5, color="black", linestyle="--", linewidth=1)
    ax.axvline(6.5, color="black", linestyle="--", linewidth=1)

    y_superior = ax.get_ylim()[1]
    ax.text(1, y_superior, "P1", ha="center", va="bottom")
    ax.text(4.5, y_superior, "AG", ha="center", va="bottom")
    ax.text(8, y_superior, "AM", ha="center", va="bottom")


def generar_boxplot(ruta_csv, titulo, ruta_salida):
    datos = preparar_datos(ruta_csv)

    crear_directorio_salida(ruta_salida)
    sns.set_theme(style="whitegrid")
    plt.figure(figsize=(15, 7))

    # Dibuja la distribucion de las 50 ejecuciones de cada algoritmo.
    ax = sns.boxplot(
        data=datos,
        x="algoritmo",
        y="fitness_2015_2024",
        hue="algoritmo",
        order=ORDEN_ALGORITMOS,
        hue_order=ORDEN_ALGORITMOS,
        palette=COLORES,
        legend=False,
        showmeans=False,
    )

    anadir_separadores(ax)

    ax.set_title(titulo)
    ax.set_xlabel("Algoritmo")
    ax.set_ylabel("Fitness 2015-2024")

    plt.xticks(rotation=45, ha="right")
    plt.tight_layout()
    # Guarda la grafica ya preparada.
    plt.savefig(ruta_salida, dpi=300)
    plt.close()


def main():
    for ruta_csv, titulo, ruta_salida in DATASETS:
        generar_boxplot(ruta_csv, titulo, ruta_salida)

    print("Graficas en graficas/boxplots/")


if __name__ == "__main__":
    main()
