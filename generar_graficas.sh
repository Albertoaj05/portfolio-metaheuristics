#!/bin/bash
set -e

echo "Generando gráficas y tablas..."
python3 scripts/boxplots.py
python3 scripts/boxplot_grasp.py
python3 scripts/boxplot_ils.py
python3 scripts/convergencia.py
python3 scripts/sensibilidad_es_phi.py
python3 scripts/sensibilidad_es_mu.py
python3 scripts/wilcoxon.py
python3 scripts/tiempos.py
echo "Listo."
echo "Gráficas: graficas/"
echo "Tablas: resultados/"
