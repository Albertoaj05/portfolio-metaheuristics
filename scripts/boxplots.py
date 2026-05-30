#!/usr/bin/env python3

# Genera los boxplots principales a partir de los CSV de resultados.

import os
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


DATASETS = [
    ("resultados/boxplot_ibex_35.csv", "Resultados IBEX 35", "graficas/boxplots/ibex_35.png"),
    ("resultados/boxplot_syp_100.csv", "Resultados S&P 100", "graficas/boxplots/syp_100.png"),
    ("resultados/boxplot_syp_500.csv", "Resultados S&P 500", "graficas/boxplots/syp_500.png"),
]

NOMBRES_ALGORITMOS = {
    "Greedy": "GREEDY",
    "BL": "BL",
    "AM-Best": "AM-BEST",
    "BMB": "BMB",
    "ILS-BL": "ILS-BL",
    "ES": "ES",
    "ILS-ES": "ILS-ES",
}

# Random no se mete en boxplot porque lo indicó el profesor.
ORDEN_ALGORITMOS = [
    "GREEDY",
    "BL",
    "AM-BEST",
    "BMB",
    "ILS-BL",
    "ES",
    "ILS-ES",
]

COLORES = {
    "GREEDY": "#B0B0B0",
    "BL": "#4C72B0",
    "AM-BEST": "#6A5ACD",
    "BMB": "#55A868",
    "ILS-BL": "#2E8B57",
    "ES": "#C44E52",
    "ILS-ES": "#8B0000",
}


def preparar_datos(ruta_csv):
    # Normaliza nombres y deja solo los algoritmos que van al boxplot principal.
    datos = pd.read_csv(ruta_csv)
    datos["algoritmo"] = datos["algoritmo"].replace(NOMBRES_ALGORITMOS)
    datos = datos[datos["algoritmo"].isin(ORDEN_ALGORITMOS)]
    return datos


def crear_directorio_salida(ruta_salida):
    os.makedirs(os.path.dirname(ruta_salida), exist_ok=True)


def generar_boxplot(ruta_csv, titulo, ruta_salida):
    # Cada dataset genera una imagen independiente.
    datos = preparar_datos(ruta_csv)

    crear_directorio_salida(ruta_salida)
    sns.set_theme(style="whitegrid")
    plt.figure(figsize=(12, 6))

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

    ax.set_title(titulo)
    ax.set_xlabel("Algoritmo")
    ax.set_ylabel("Fitness 2015-2024")

    plt.xticks(rotation=35, ha="right")
    plt.tight_layout()
    plt.savefig(ruta_salida, dpi=300)
    plt.close()


def main():
    for ruta_csv, titulo, ruta_salida in DATASETS:
        generar_boxplot(ruta_csv, titulo, ruta_salida)



if __name__ == "__main__":
    main()
