# Entrenamiento del identificador

El firmware `cam_identificador` no entrena en la ESP32-CAM. La placa solo corre
inferencia con el modelo que esta embebido en:

- `../cam_identificador/code/main/identifier_detect_model_data.cc`
- `../cam_identificador/code/main/identifier_detect_model_data.h`

Por eso, antes de flashear un detector propio, hay que entrenar en el PC, exportar
un `.tflite` cuantizado y regenerar esos archivos C.

Para la habilitacion contra el dummy se usa el flujo full-frame de 4 clases:

```text
none
identifier_left
identifier_center
identifier_right
```

Ese flujo entrena con la foto completa y permite que el robot sepa hacia donde
girar hasta centrar el dummy. El entrenamiento por cuadrantes queda como
referencia historica y alternativa experimental.

## Estado del dataset actual

El dataset esperado esta un nivel sobre `labs`:

```bash
cd ~/se-labs/labs/Proyecto
python3 training/inspect_dataset.py --dataset ../../dataset_embebidos_grupo3
```

Resultado observado despues de agregar fotos negativas y de derecha:

```text
celular: 277 imagenes, 276 positivas, 1 negativa
esp:     244 imagenes, 193 positivas, 51 negativas
```

Para full-frame, las fotos ESP actuales permiten entrenar las cuatro clases:

```text
none: 51
identifier_left: 34
identifier_center: 97
identifier_right: 62
```

Si el resultado es inestable, conviene seguir agregando fotos ESP reales:

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

## Entrenar modelo full-frame compatible con el firmware

Para el firmware actual de habilitacion usa primero el entrenamiento full-frame:

```bash
cd ~/se-labs/labs/Proyecto
source .venv-train/bin/activate
python training/train_identifier_full.py \
  --dataset ../../dataset_embebidos_grupo3 \
  --sources esp \
  --epochs 80 \
  --emit-firmware
```

El script genera:

- `training/out_full/identifier_full_model.keras`: modelo Keras entrenado.
- `training/out_full/identifier_detect_model.tflite`: modelo TFLite int8.
- `training/out_full/identifier_detect_model_data.cc`: arreglo C generado.
- `training/out_full/metrics.json`: metricas y matriz de confusion.

Con `--emit-firmware`, el script reemplaza automaticamente el modelo embebido en
`cam_identificador/code/main/`.

Resultado observado con el dataset ESP actual:

```text
TFLite accuracy: 0.8333
Validacion: 48 imagenes
Matriz de confusion:
  none:              9 none, 0 left, 1 center, 0 right
  identifier_left:   0 none, 6 left, 1 center, 0 right
  identifier_center: 0 none, 3 left, 15 center, 1 right
  identifier_right:  0 none, 0 left, 2 center, 10 right
```

Esto es suficiente para probar la estrategia de habilitacion, pero aun puede
mejorar agregando mas fotos de izquierda y centro con variaciones de distancia,
luz y fondo real del ring.

La telemetria del firmware full-frame queda:

```text
IDENTIFIER,detected=1,zone=center,none_score=2,left_score=7,center_score=91,right_score=0,best_score=91,threshold=65
```

## Entrenar modelo por cuadrante alternativo

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

La version actual para habilitacion entrena cuatro clases con la foto completa:

```text
none
identifier_left
identifier_center
identifier_right
```
