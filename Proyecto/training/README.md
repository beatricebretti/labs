# Entrenamiento del identificador

El firmware `cam_identificador` no entrena en la ESP32-CAM. La placa solo corre
inferencia con el modelo que esta embebido en:

- `../cam_identificador/code/main/identifier_detect_model_data.cc`
- `../cam_identificador/code/main/identifier_detect_model_data.h`

Por eso, antes de flashear un detector propio, hay que entrenar en el PC, exportar
un `.tflite` cuantizado y regenerar esos archivos C.

## Estado del dataset actual

El dataset esperado esta un nivel sobre `labs`:

```bash
cd ~/se-labs/labs/Proyecto
python3 training/inspect_dataset.py --dataset ../../dataset_embebidos_grupo3
```

Resultado observado en este checkout para imagen completa:

```text
celular: 277 imagenes, 276 positivas, 1 negativa
esp:     145 imagenes, 144 positivas, 1 negativa
```

Eso no alcanza para entrenar un clasificador binario de imagen completa. Por eso
este flujo entrena por cuadrante: cada imagen se divide en `left`, `center` y
`right`. Si la etiqueta dice que el identificador esta en el cuadrante 2, ese
crop se vuelve positivo y los cuadrantes 1 y 3 se vuelven negativos.

Con las fotos ESP actuales, la expansion por cuadrantes produce:

```text
144 crops positivos
291 crops negativos
```

Aunque ya se puede entrenar con esos crops, conviene agregar fotos negativas
tomadas con la ESP para robustez:

- ring sin robot;
- borde del ring;
- suelo/mesa/fondo del laboratorio;
- robot sin identificador visible;
- partes del robot que no son el identificador;
- identificador fuera de cuadro o muy parcial, si quieren que eso cuente como
  `no_identifier`.

Mejor aun si tienen variacion de distancia, luz, orientacion y fondo.

## Instalacion en WSL

Crear un entorno Python separado:

```bash
cd ~/se-labs/labs/Proyecto
python3 -m venv .venv-train
source .venv-train/bin/activate
python -m pip install --upgrade pip
python -m pip install -r training/requirements.txt
```

Verificacion:

```bash
python - <<'PY'
import tensorflow as tf
import numpy as np
print("tensorflow", tf.__version__)
print("numpy", np.__version__)
PY
```

## Entrenar modelo por cuadrante compatible con el firmware

Entrenar con las fotos ESP:

```bash
cd ~/se-labs/labs/Proyecto
source .venv-train/bin/activate
python training/train_identifier.py \
  --dataset ../../dataset_embebidos_grupo3 \
  --sources esp \
  --epochs 60 \
  --emit-firmware
```

Por que `--sources esp`: el modelo final debe parecerse a lo que vera la
ESP32-CAM en combate. Las fotos de celular pueden servir para experimentos, pero
pueden meter una distribucion distinta a la camara real.

El script genera:

- `training/out/identifier_model.keras`: modelo Keras entrenado.
- `training/out/identifier_detect_model.tflite`: modelo TFLite int8.
- `training/out/identifier_detect_model_data.cc`: arreglo C generado.
- `training/out/metrics.json`: metricas y matriz de confusion.

Con `--emit-firmware`, el script reemplaza automaticamente el modelo embebido en
`cam_identificador/code/main/`.

El firmware final captura una imagen de 96x96, la divide en tres zonas verticales,
estira cada zona a 96x96 y corre el mismo modelo tres veces. La telemetria queda:

```text
IDENTIFIER,detected=1,zone=center,left_score=12,center_score=93,right_score=9,best_score=93,threshold=70
```

Luego compila y flashea:

```bash
source ~/esp/esp-idf/export.sh
cd ~/se-labs/labs/Proyecto/cam_identificador/code
idf.py -p "$ESPPORT" build flash monitor
```

## Como se usa el cuadrante

El archivo `etiquetas.txt` trae un tercer campo de cuadrante. El modelo sigue
siendo binario:

```text
no_identifier
identifier
```

La ubicacion sale de correr el modelo una vez por zona. No se entrenan cuatro
clases. Esto mantiene el modelo pequeno y aprovecha los cuadrantes no ocupados
como negativos.

Una alternativa futura seria entrenar cuatro clases:

```text
none
identifier_left
identifier_center
identifier_right
```

Eso requiere cambiar `model_settings.h`, `model_settings.cc`,
`detection_responder.cc` y la logica que imprime telemetria.
