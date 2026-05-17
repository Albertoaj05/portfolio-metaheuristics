#!/usr/bin/env python3

# Genera boxplots para comparar los cruces aritmetico, BLX y uniforme.

import os

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


DATASETS = [
    ("resultados/boxplot_ibex_35.csv", "Cruces - IBEX 35", "graficas/cruces/ibex_35.png"),
    ("resultados/boxplot_syp_100.csv", "Cruces - S&P 100", "graficas/cruces/syp_100.png"),
    ("resultados/boxplot_syp_500.csv", "Cruces - S&P 500", "graficas/cruces/syp_500.png"),
]

# Traduce nombres internos a etiquetas mas legibles en la grafica.
NOMBRES_ALGORITMOS = {
    "AGG-Arit": "AGG-ARIT",
    "AGG-BLX": "AGG-BLX",
    "AGG-Unif": "AGG-UNIF",
    "AGE-Arit": "AGE-ARIT",
    "AGE-BLX": "AGE-BLX",
    "AGE-Unif": "AGE-UNIF",
}

ORDEN_ALGORITMOS = [
    "AGG-ARIT",
    "AGG-BLX",
    "AGG-UNIF",
    "AGE-ARIT",
    "AGE-BLX",
    "AGE-UNIF",
]

COLORES = {
    "AGG-ARIT": "#55A868",
    "AGG-BLX": "#2E8B57",
    "AGG-UNIF": "#1E5631",
    "AGE-ARIT": "#C44E52",
    "AGE-BLX": "#8B0000",
    "AGE-UNIF": "#5C0000",
}

PROPIEDADES_MEDIA = {
    "marker": "D",
    "markerfacecolor": "white",
    "markeredgecolor": "black",
    "markersize": 6,
}


def preparar_datos(ruta_csv):
    # Carga resultados y se queda solo con las variantes de cruce.
    datos = pd.read_csv(ruta_csv)
    datos["algoritmo"] = datos["algoritmo"].replace(NOMBRES_ALGORITMOS)
    datos = datos[datos["algoritmo"].isin(ORDEN_ALGORITMOS)]
    return datos


def crear_directorio_salida(ruta_salida):
    directorio = os.path.dirname(ruta_salida)
    os.makedirs(directorio, exist_ok=True)


def anadir_separadores(ax):
    # Marca la separacion entre modelos AGG y AGE.
    ax.axvline(2.5, color="black", linestyle="--", linewidth=1)

    y_superior = ax.get_ylim()[1]
    ax.text(1, y_superior, "AGG", ha="center", va="bottom")
    ax.text(4.5, y_superior, "AGE", ha="center", va="bottom")


def generar_boxplot(ruta_csv, titulo, ruta_salida):
    datos = preparar_datos(ruta_csv)

    if datos.empty:
        print(f"AVISO: sin datos en {ruta_csv}")
        return

    crear_directorio_salida(ruta_salida)
    sns.set_theme(style="whitegrid")
    plt.figure(figsize=(11, 6))

    # Muestra la dispersion de fitness para cada operador de cruce.
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
    ax.set_xlabel("Algoritmo")
    ax.set_ylabel("Fitness 2015-2024")

    plt.xticks(rotation=30, ha="right")
    plt.tight_layout()
    # Exporta la figura final.
    plt.savefig(ruta_salida, dpi=300)
    plt.close()

    print(f"Generado: {ruta_salida}")


def main():
    for ruta_csv, titulo, ruta_salida in DATASETS:
        generar_boxplot(ruta_csv, titulo, ruta_salida)


if __name__ == "__main__":
    main()
