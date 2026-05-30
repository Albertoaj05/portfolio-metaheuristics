#!/usr/bin/env python3

# Compara GRASP con los metodos basados en BL mas cercanos.

import os

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


DATASETS = [
    ("resultados/boxplot_ibex_35.csv", "IBEX 35"),
    ("resultados/boxplot_syp_100.csv", "S&P 100"),
    ("resultados/boxplot_syp_500.csv", "S&P 500"),
]

ORDEN = ["BL", "BMB", "BMB-10", "GRASP", "ILS-BL"]

COLORES = {
    "BL": "#4C72B0",
    "BMB": "#55A868",
    "BMB-10": "#2E8B57",
    "GRASP": "#1E5631",
    "ILS-BL": "#6A5ACD",
}


def leer_datos(ruta_csv, titulo):
    # Filtra el CSV para quedarse solo con los algoritmos de esta comparacion.
    if not os.path.exists(ruta_csv):
        print(f"AVISO: no existe {ruta_csv}")
        return None

    datos = pd.read_csv(ruta_csv)
    datos = datos[datos["algoritmo"].isin(ORDEN)]

    if datos.empty:
        print(f"AVISO: sin datos para {titulo}")
        return None

    return datos


def dibujar(ax, datos, titulo):
    sns.boxplot(
        data=datos,
        x="algoritmo",
        y="fitness_2015_2024",
        hue="algoritmo",
        order=ORDEN,
        hue_order=ORDEN,
        palette=COLORES,
        legend=False,
        showmeans=True,
        meanprops={"marker": "D", "markerfacecolor": "white", "markeredgecolor": "black", "markersize": 5},
        ax=ax,
    )

    ax.set_title(titulo)
    ax.set_xlabel("Algoritmo")
    ax.set_ylabel("Fitness 2015-2024")
    ax.tick_params(axis="x", rotation=25)


def main():
    # Se dibujan los tres datasets en una misma figura.
    sns.set_theme(style="whitegrid")

    fig, axes = plt.subplots(1, 3, figsize=(18, 5.2), sharey=False)

    for ax, (ruta_csv, titulo) in zip(axes, DATASETS):
        datos = leer_datos(ruta_csv, titulo)
        if datos is not None:
            dibujar(ax, datos, titulo)

    fig.suptitle("GRASP frente a métodos con BL", fontsize=13)
    plt.tight_layout(rect=(0, 0, 1, 0.94))
    ruta_conjunta = "graficas/comparacionGrasp/comparacionGrasp.png"
    os.makedirs(os.path.dirname(ruta_conjunta), exist_ok=True)
    plt.savefig(ruta_conjunta, dpi=300)
    plt.close()


if __name__ == "__main__":
    main()
