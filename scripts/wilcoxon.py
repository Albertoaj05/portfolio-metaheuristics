#!/usr/bin/env python3

# Calcula tests de Wilcoxon pareados y genera un resumen en CSV y grafica.

import csv
import os

import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from scipy.stats import wilcoxon


PARES_COMPARACION = [
    ("AGG-BLX", "AGG-Arit"),
    ("AGE-BLX", "AGE-Arit"),
    ("AGG-BLX", "AGG-Unif"),
    ("AGG-Unif", "AGG-Arit"),
    ("AM-Best", "AGG-BLX"),
    ("AM-Best", "AM-Rand"),
    ("AM-Best", "AM-All"),
    ("AM-Rand", "AM-All"),
    ("AM-Best", "AM-Best1"),
    ("BL", "Random"),
    ("AGG-BLX", "BL"),
]

# Datasets usados para leer los resultados de las 50 ejecuciones.
DATASETS = [
    ("ibex_35", "Ibex 35"),
    ("syp_100", "S&P 100"),
    ("syp_500", "S&P 500"),
]

RUTA_CSV_SALIDA = "resultados/wilcoxon.csv"
RUTA_GRAFICA_SALIDA = "graficas/wilcoxon/wilcoxon.png"


def leer_boxplot(ruta_csv):
    # Lee los fitness individuales de cada algoritmo.
    datos = {}

    with open(ruta_csv) as fichero:
        lector = csv.DictReader(fichero)

        for fila in lector:
            algoritmo = fila["algoritmo"]
            fitness = float(fila["fitness_2015_2024"])

            if algoritmo not in datos:
                datos[algoritmo] = []

            datos[algoritmo].append(fitness)

    return datos


def media(valores):
    return sum(valores) / len(valores)


def significancia(p_valor):
    # Convierte el p-valor en una marca de significancia.
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


def calcular_wilcoxon(valores_a, valores_b):
    # Contraste unilateral: comprueba si A supera a B.
    try:
        _, p_valor = wilcoxon(valores_a, valores_b, alternative="greater")
    except ValueError:
        return None

    return p_valor


def calcular_resultados():
    # Calcula todos los pares y prepara datos para CSV y tabla.
    resultados_por_par = {}
    filas_csv = [
        [
            "dataset",
            "algoritmo_A",
            "algoritmo_B",
            "media_A",
            "media_B",
            "p_valor",
            "significancia",
        ]
    ]

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

            p_valor = calcular_wilcoxon(valores_a, valores_b)

            if p_valor is None:
                continue

            filas_csv.append(
                [
                    nombre_dataset,
                    algoritmo_a,
                    algoritmo_b,
                    f"{media(valores_a):.6f}",
                    f"{media(valores_b):.6f}",
                    f"{p_valor:.6f}",
                    significancia(p_valor),
                ]
            )

            par = (algoritmo_a, algoritmo_b)

            if par not in resultados_por_par:
                resultados_por_par[par] = {}

            resultados_por_par[par][nombre_dataset] = p_valor

    return filas_csv, resultados_por_par


def guardar_csv(filas_csv):
    # Guarda la tabla numerica con medias, p-valor y significancia.
    with open(RUTA_CSV_SALIDA, "w", newline="") as fichero:
        escritor = csv.writer(fichero)
        escritor.writerows(filas_csv)


def construir_tabla(resultados_por_par):
    # Prepara textos y colores para la tabla dibujada con matplotlib.
    texto_tabla = []
    colores_tabla = []

    for algoritmo_a, algoritmo_b in PARES_COMPARACION:
        fila_texto = [f"{algoritmo_a} vs {algoritmo_b}"]
        fila_colores = ["#ffffff"]

        for _, nombre_dataset in DATASETS:
            p_valor = resultados_por_par.get((algoritmo_a, algoritmo_b), {}).get(nombre_dataset)

            if p_valor is None:
                fila_texto.append("-")
                fila_colores.append("#eeeeee")
            else:
                fila_texto.append(f"{p_valor:.4f} {significancia(p_valor)}")
                fila_colores.append(color_significancia(p_valor))

        texto_tabla.append(fila_texto)
        colores_tabla.append(fila_colores)

    return texto_tabla, colores_tabla


def guardar_grafica(resultados_por_par):
    columnas = ["Comparacion"] + [nombre for _, nombre in DATASETS]
    texto_tabla, colores_tabla = construir_tabla(resultados_por_par)

    os.makedirs(os.path.dirname(RUTA_GRAFICA_SALIDA), exist_ok=True)

    figura, ax = plt.subplots(figsize=(11, 5.6))
    ax.axis("off")

    # Dibuja la tabla con colores segun la significancia.
    tabla = ax.table(
        cellText=texto_tabla,
        colLabels=columnas,
        cellColours=colores_tabla,
        cellLoc="center",
        loc="center",
        colWidths=[0.35, 0.22, 0.22, 0.22],
    )

    tabla.auto_set_font_size(False)
    tabla.set_fontsize(10)
    tabla.scale(1, 1.5)

    for columna in range(len(columnas)):
        tabla[0, columna].set_facecolor("#cfd8dc")
        tabla[0, columna].set_text_props(weight="bold")

    for fila in range(len(texto_tabla)):
        tabla[fila + 1, 0].set_text_props(weight="bold")

    leyenda = [
        Patch(facecolor="#a8e6a3", edgecolor="black", label="Verde fuerte: muy significativo (p < 0.001)"),
        Patch(facecolor="#cdedca", edgecolor="black", label="Verde medio: significativo (p < 0.01)"),
        Patch(facecolor="#e6f4e3", edgecolor="black", label="Verde claro: significativo (p < 0.05)"),
        Patch(facecolor="#f4d4d4", edgecolor="black", label="Rojo: no significativo"),
    ]

    figura.legend(
        handles=leyenda,
        loc="lower center",
        ncol=2,
        frameon=False,
        fontsize=9,
        bbox_to_anchor=(0.5, 0.02),
    )

    plt.tight_layout(rect=[0, 0.07, 1, 1])
    plt.savefig(RUTA_GRAFICA_SALIDA, dpi=140, bbox_inches="tight")
    plt.close(figura)

    print(f"Generado: {RUTA_GRAFICA_SALIDA}")


def main():
    filas_csv, resultados_por_par = calcular_resultados()
    guardar_csv(filas_csv)
    guardar_grafica(resultados_por_par)


if __name__ == "__main__":
    main()
