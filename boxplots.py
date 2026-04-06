import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Función para generar un boxplot a partir de un CSV con resultados
def generar_boxplot(csv_file, titulo, output_png):
    
    # Cargamos los datos
    df = pd.read_csv(csv_file)

    # Modificamos los nombres para que se vean mejor en la gráfica
    df["algoritmo"] = df["algoritmo"].replace({
        "Random": "RANDOM",
        "Greedy": "GREEDY",
        "BL": "BL"
    })

    # Asignamos un estilo básico para que quede más limpio
    sns.set(style="whitegrid")
    plt.figure(figsize=(8, 5))

    # Creamos el boxplot comparando los algoritmos
    ax = sns.boxplot(
        data=df,
        x="algoritmo",
        y="fitness_2015_2024",
        order=["GREEDY", "RANDOM", "BL"]  # fijo el orden para comparar mejor
    )

    # Título y etiquetas
    ax.set_title(titulo)
    ax.set_xlabel("Algoritmo")
    ax.set_ylabel("Fitness")

    # Ajustamos márgenes y guardamos la imagen
    plt.tight_layout()
    plt.savefig(output_png)
    plt.close()


# Generamos los gráficos para cada dataset
generar_boxplot(
    "resultados/boxplot_ibex_35.csv",
    "Resultados IBEX 35 por algoritmo",
    "graficas/ibex_35_results.png"
)

generar_boxplot(
    "resultados/boxplot_syp_100.csv",
    "Resultados S&P 100 por algoritmo",
    "graficas/syp_100_results.png"
)

generar_boxplot(
    "resultados/boxplot_syp_500.csv",
    "Resultados S&P 500 por algoritmo",
    "graficas/syp_500_results.png"
)

print("Gráficas generadas correctamente.")