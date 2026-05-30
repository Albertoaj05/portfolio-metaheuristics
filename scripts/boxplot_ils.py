#!/usr/bin/env python3

# Compara las variantes de ILS-BL e ILS-ES por porcentaje de mutacion.

import os

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


DATASETS = [
    ("resultados/boxplot_ibex_35.csv", "IBEX 35"),
    ("resultados/boxplot_syp_100.csv", "S&P 100"),
    ("resultados/boxplot_syp_500.csv", "S&P 500"),
]

ORDEN = ["ILS-10", "ILS-BL", "ILS-30", "ILS-ES-10", "ILS-ES", "ILS-ES-30"]

COLORES = {
    "ILS-10": "#9ecae1",
    "ILS-BL": "#2E8B57",
    "ILS-30": "#08519c",
    "ILS-ES-10": "#fdd0a2",
    "ILS-ES": "#c44e52",
    "ILS-ES-30": "#a63603",
}

ETIQUETAS = {
    "ILS-10": "ILS-BL-10",
    "ILS-BL": "ILS-BL-20",
    "ILS-30": "ILS-BL-30",
    "ILS-ES-10": "ILS-ES-10",
    "ILS-ES": "ILS-ES-20",
    "ILS-ES-30": "ILS-ES-30",
}


def leer_datos(ruta_csv, titulo):
    # Filtra las variantes de ILS que se estudian como trabajo extra.
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
    ax.set_xlabel("Variante")
    ax.set_ylabel("Fitness 2015-2024")
    ax.set_xticks(range(len(ORDEN)))
    ax.set_xticklabels([ETIQUETAS[nombre] for nombre in ORDEN], rotation=25, ha="right")


def main():
    # Se agrupan los tres datasets para comparar visualmente las variantes.
    sns.set_theme(style="whitegrid")

    fig, axes = plt.subplots(1, 3, figsize=(19, 5.2), sharey=False)

    for ax, (ruta_csv, titulo) in zip(axes, DATASETS):
        datos = leer_datos(ruta_csv, titulo)
        if datos is not None:
            dibujar(ax, datos, titulo)

    fig.suptitle("Variantes ILS e ILS-ES", fontsize=13)
    plt.tight_layout(rect=(0, 0, 1, 0.94))
    ruta_conjunta = "graficas/comparacionILS/comparacionILS.png"
    os.makedirs(os.path.dirname(ruta_conjunta), exist_ok=True)
    plt.savefig(ruta_conjunta, dpi=300)
    plt.close()


if __name__ == "__main__":
    main()
