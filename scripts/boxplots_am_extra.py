#!/usr/bin/env python3

# Genera boxplots para comparar las variantes memeticas y AM-Best1.

import os

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


DATASETS = [
    ("resultados/boxplot_ibex_35.csv", "Variantes AM - IBEX 35", "graficas/am_extra/ibex_35.png"),
    ("resultados/boxplot_syp_100.csv", "Variantes AM - S&P 100", "graficas/am_extra/syp_100.png"),
    ("resultados/boxplot_syp_500.csv", "Variantes AM - S&P 500", "graficas/am_extra/syp_500.png"),
]

# Nombres y orden usados en la comparacion de variantes AM.
NOMBRES_ALGORITMOS = {
    "AM-All": "AM-ALL",
    "AM-Rand": "AM-RAND",
    "AM-Best": "AM-BEST",
    "AM-Best1": "AM-BEST1",
}

ORDEN_ALGORITMOS = [
    "AM-ALL",
    "AM-RAND",
    "AM-BEST",
    "AM-BEST1",
]

COLORES = {
    "AM-ALL": "#8172B3",
    "AM-RAND": "#9370DB",
    "AM-BEST": "#6A5ACD",
    "AM-BEST1": "#483D8B",
}

PROPIEDADES_MEDIA = {
    "marker": "D",
    "markerfacecolor": "white",
    "markeredgecolor": "black",
    "markersize": 6,
}


def preparar_datos(ruta_csv):
    # Carga el CSV y filtra solo las variantes memeticas.
    datos = pd.read_csv(ruta_csv)
    datos["algoritmo"] = datos["algoritmo"].replace(NOMBRES_ALGORITMOS)
    datos = datos[datos["algoritmo"].isin(ORDEN_ALGORITMOS)]
    return datos


def crear_directorio_salida(ruta_salida):
    directorio = os.path.dirname(ruta_salida)
    os.makedirs(directorio, exist_ok=True)


def anadir_separadores(ax):
    # Separa las variantes obligatorias de la variante extra.
    ax.axvline(2.5, color="black", linestyle="--", linewidth=1)

    y_superior = ax.get_ylim()[1]
    ax.text(1, y_superior, "Obligatorio", ha="center", va="bottom", fontsize=9)
    ax.text(3, y_superior, "Extra", ha="center", va="bottom", fontsize=9)


def generar_boxplot(ruta_csv, titulo, ruta_salida):
    datos = preparar_datos(ruta_csv)

    if datos.empty:
        print(f"AVISO: sin datos en {ruta_csv}")
        return

    crear_directorio_salida(ruta_salida)
    sns.set_theme(style="whitegrid")
    plt.figure(figsize=(9, 6))

    # Dibuja la distribucion de fitness de cada variante memetica.
    ax = sns.boxplot(
        data=datos,
        x="algoritmo",
        y="fitness_2015_2024",
        hue="algoritmo",
        order=ORDEN_ALGORITMOS,
        hue_order=ORDEN_ALGORITMOS,
        palette=COLORES,
        legend=False,
        showmeans=True,
        meanprops=PROPIEDADES_MEDIA,
    )

    anadir_separadores(ax)

    ax.set_title(titulo)
    ax.set_xlabel("Variante AM")
    ax.set_ylabel("Fitness 2015-2024")

    plt.xticks(rotation=20, ha="right")
    plt.tight_layout()
    # Guarda la grafica en su carpeta.
    plt.savefig(ruta_salida, dpi=300)
    plt.close()

    print(f"Generado: {ruta_salida}")


def main():
    for ruta_csv, titulo, ruta_salida in DATASETS:
        generar_boxplot(ruta_csv, titulo, ruta_salida)


if __name__ == "__main__":
    main()
