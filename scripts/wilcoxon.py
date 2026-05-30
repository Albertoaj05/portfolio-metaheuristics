#!/usr/bin/env python3

# Calcula el Wilcoxon pareado y genera la tabla resumen.

import csv
import os

import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from scipy.stats import wilcoxon


PARES_COMPARACION = [
    ("ILS-BL", "BMB"),
    ("BMB", "ES"),
    ("ILS-BL", "ES"),
    ("ILS-ES", "BMB"),
    ("ILS-ES", "ILS-BL"),
    ("ILS-ES", "ES"),
]

DATASETS = [
    ("ibex_35", "Ibex 35"),
    ("syp_100", "S&P 100"),
    ("syp_500", "S&P 500"),
]

RUTA_CSV_SALIDA = "resultados/wilcoxon.csv"
RUTA_GRAFICA_SALIDA = "graficas/wilcoxon/wilcoxon.png"


def leer_boxplot(ruta_csv):
    datos = {}

    with open(ruta_csv) as fichero:
        lector = csv.DictReader(fichero)

        for fila in lector:
            algoritmo = fila["algoritmo"]
            fitness = float(fila["fitness_2015_2024"])
            datos.setdefault(algoritmo, []).append(fitness)

    return datos


def media(valores):
    return sum(valores) / len(valores)


def significancia(p_valor):
    if p_valor < 0.001:
        return "***"
    if p_valor < 0.01:
        return "**"
    if p_valor < 0.05:
        return "*"
    return "n.s."


def color_significancia(p_valor):
    if p_valor < 0.001:
        return "#a8e6a3"
    if p_valor < 0.01:
        return "#cdedca"
    if p_valor < 0.05:
        return "#e6f4e3"
    return "#f4d4d4"


def calcular_resultados():
    # Compara las 50 ejecuciones emparejadas usando las mismas semillas.
    resultados_por_par = {}
    filas_csv = [["dataset", "algoritmo_A", "algoritmo_B", "media_A", "media_B", "p_valor", "significancia"]]

    for sufijo, nombre_dataset in DATASETS:
        ruta_csv = f"resultados/boxplot_{sufijo}.csv"
        if not os.path.exists(ruta_csv):
            continue

        datos = leer_boxplot(ruta_csv)

        for algoritmo_a, algoritmo_b in PARES_COMPARACION:
            if algoritmo_a not in datos or algoritmo_b not in datos:
                continue

            valores_a = datos[algoritmo_a]
            valores_b = datos[algoritmo_b]
            if len(valores_a) != len(valores_b):
                continue

            try:
                _, p_valor = wilcoxon(valores_a, valores_b, alternative="greater")
            except ValueError:
                continue

            resultados_por_par.setdefault((algoritmo_a, algoritmo_b), {})[nombre_dataset] = p_valor
            filas_csv.append([
                nombre_dataset,
                algoritmo_a,
                algoritmo_b,
                f"{media(valores_a):.6f}",
                f"{media(valores_b):.6f}",
                f"{p_valor:.6g}",
                significancia(p_valor),
            ])

    return resultados_por_par, filas_csv


def guardar_csv(filas):
    os.makedirs(os.path.dirname(RUTA_CSV_SALIDA), exist_ok=True)
    with open(RUTA_CSV_SALIDA, "w", newline="") as fichero:
        writer = csv.writer(fichero)
        writer.writerows(filas)


def guardar_grafica(resultados_por_par):
    # Pinta una tabla de p-valores con colores segun la significancia.
    if not resultados_por_par:
        print("AVISO: no hay datos para Wilcoxon")
        return

    pares = list(resultados_por_par.keys())
    datasets = [nombre for _, nombre in DATASETS]

    columnas = ["Comparacion"] + datasets
    texto_tabla = []
    colores_tabla = []

    for algoritmo_a, algoritmo_b in pares:
        fila_texto = [f"{algoritmo_a} vs {algoritmo_b}"]
        fila_colores = ["#f7f7f7"]

        for dataset in datasets:
            p_valor = resultados_por_par[(algoritmo_a, algoritmo_b)].get(dataset)

            if p_valor is None:
                fila_texto.append("-")
                fila_colores.append("#eeeeee")
            else:
                fila_texto.append(f"{p_valor:.4f} {significancia(p_valor)}")
                fila_colores.append(color_significancia(p_valor))

        texto_tabla.append(fila_texto)
        colores_tabla.append(fila_colores)

    fig, ax = plt.subplots(figsize=(12.5, 0.48 * len(pares) + 2.0))
    ax.axis("off")

    tabla = ax.table(
        cellText=texto_tabla,
        colLabels=columnas,
        cellColours=colores_tabla,
        colColours=["#cfd8dc"] * len(columnas),
        colWidths=[0.35, 0.22, 0.22, 0.22],
        cellLoc="center",
        loc="upper center",
        bbox=[0.02, 0.24, 0.96, 0.72],
    )

    tabla.auto_set_font_size(False)
    tabla.set_fontsize(10)
    tabla.scale(1, 1.25)

    for (fila, columna), celda in tabla.get_celld().items():
        celda.set_edgecolor("black")
        celda.set_linewidth(0.9)

        if fila == 0:
            celda.set_text_props(weight="bold")
        elif columna == 0:
            celda.set_text_props(weight="bold")

    leyenda = [
        Patch(facecolor="#a8e6a3", edgecolor="black", label="Verde fuerte: muy significativo (p < 0.001)"),
        Patch(facecolor="#cdedca", edgecolor="black", label="Verde medio: significativo (p < 0.01)"),
        Patch(facecolor="#e6f4e3", edgecolor="black", label="Verde claro: significativo (p < 0.05)"),
        Patch(facecolor="#f4d4d4", edgecolor="black", label="Rojo: no significativo"),
    ]
    ax.legend(handles=leyenda, loc="lower center", bbox_to_anchor=(0.5, 0.02), ncol=2, frameon=False)

    os.makedirs(os.path.dirname(RUTA_GRAFICA_SALIDA), exist_ok=True)
    plt.subplots_adjust(top=0.98, bottom=0.08, left=0.02, right=0.98)
    plt.savefig(RUTA_GRAFICA_SALIDA, dpi=220)
    plt.close()


def main():
    resultados, filas = calcular_resultados()
    guardar_csv(filas)
    guardar_grafica(resultados)


if __name__ == "__main__":
    main()
