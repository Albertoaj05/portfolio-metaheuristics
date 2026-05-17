#!/bin/bash
echo "Generando gráficas y tablas..."
python3 scripts/boxplots.py
python3 scripts/convergencia.py
python3 scripts/sensibilidad_alpha.py
python3 scripts/wilcoxon.py
python3 scripts/torneo_k.py
python3 scripts/tiempos.py
python3 scripts/boxplots_cruces.py
python3 scripts/boxplots_am_extra.py
echo "Listo."
echo "Gráficas: graficas/"
echo "Tablas: resultados/"