# Habilitacion dummy: buscar, centrar y empujar

Este flujo prepara el robot para la habilitacion: estando solo en el ring debe
encontrar el robot dummy con identificador, orientarse hacia el centro del dummy,
avanzar para empujarlo y evitar salirse del ring.

La solucion usa tres firmwares:

- `cam_identificador/code`: ESP32-CAM con modelo TinyML full-frame.
- `cam_bordes/code`: ESP32-CAM con filtro Sobel para detectar el masking tape.
- `habilitacion_dummy/code`: ESP32-S3 controlador de motores.

## Idea de control

Prioridad de acciones:

1. Si la camara de bordes reporta `EDGE,edge=1`, el S3 evade el borde.
2. Si el identificador esta a la izquierda, gira a la izquierda.
3. Si el identificador esta a la derecha, gira a la derecha.
4. Si el identificador esta al centro, avanza para empujar.
5. Si no ve identificador, gira buscando.

El modelo del identificador ya no se entrena por crops de cuadrantes. Se entrena
con la foto completa y cuatro clases:

```text
none
identifier_left
identifier_center
identifier_right
```

Asi el modelo usa el frame completo y aun asi entrega direccion.

## 1. Verificar dataset

Desde WSL:

```bash
cd ~/labs/Proyecto
python3 training/inspect_dataset.py --dataset ../../dataset_embebidos_grupo3
```

Por que: confirma que las fotos nuevas quedaron etiquetadas y que no faltan
archivos.

Verificacion esperada para `esp`:

```text
esp:
  imagenes etiquetadas: ...
  presentes: ...
  ausentes: ...
  cuadrantes positivos: {1: ..., 2: ..., 3: ...}
  imagenes faltantes: 0
```

Para full-frame debe haber ejemplos suficientes de:

- `present=0`: clase `none`.
- `present=1, quadrant=1`: clase `identifier_left`.
- `present=1, quadrant=2`: clase `identifier_center`.
- `present=1, quadrant=3`: clase `identifier_right`.

## 2. Crear entorno Python de entrenamiento

```bash
cd ~/labs/Proyecto
python3 -m venv .venv-train
source .venv-train/bin/activate
python -m pip install --upgrade pip
python -m pip install -r training/requirements.txt
```

Por que: el entrenamiento usa TensorFlow en el computador, no en la ESP32-CAM.

Verificacion:

```bash
python - <<'PY'
import tensorflow as tf
import numpy as np
print("tensorflow", tf.__version__)
print("numpy", np.__version__)
PY
```

Debe imprimir versiones sin error.

## 3. Entrenar modelo full-frame

```bash
cd ~/labs/Proyecto
source .venv-train/bin/activate
python training/train_identifier_full.py \
  --dataset ../../dataset_embebidos_grupo3 \
  --sources esp \
  --epochs 80 \
  --emit-firmware
```

Por que:

- `--sources esp`: entrena con la distribucion real de la ESP32-CAM.
- `--epochs 80`: da margen suficiente; el script usa early stopping.
- `--emit-firmware`: copia el `.tflite` convertido a arreglo C dentro de
  `cam_identificador/code/main`.

Verificacion:

```bash
cat training/out_full/metrics.json
ls -lh training/out_full/identifier_detect_model.tflite
ls -lh cam_identificador/code/main/identifier_detect_model_data.cc
```

Revisa en `metrics.json`:

- `mode`: debe ser `full_frame_4class`.
- `class_names`: debe mostrar `none`, `identifier_left`,
  `identifier_center`, `identifier_right`.
- `tflite.accuracy`: idealmente sobre `0.75`. Si queda mucho mas bajo, faltan
  fotos o hay etiquetas equivocadas.

## 4. Flashear ESP32-CAM del identificador

Conecta solo la ESP32-CAM del identificador al computador.

```bash
cd ~/labs/Proyecto/cam_identificador/code
get_esp32
export ESPPORT=/dev/ttyUSB0
idf.py set-target esp32
idf.py -p "$ESPPORT" flash monitor
```

Si tu puerto no es `/dev/ttyUSB0`:

```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
export ESPPORT=/dev/ttyUSBX
```

Verificacion en monitor:

```text
IDENTIFIER,detected=0,zone=none,...
IDENTIFIER,detected=1,zone=left,...
IDENTIFIER,detected=1,zone=center,...
IDENTIFIER,detected=1,zone=right,...
```

Prueba poniendo el dummy a izquierda, centro y derecha de la camara. La zona debe
cambiar de forma coherente.

Salir del monitor:

```text
Ctrl+]
```

## 5. Flashear ESP32-CAM de bordes

Conecta solo la ESP32-CAM de bordes al computador.

```bash
cd ~/labs/Proyecto/cam_bordes/code
get_esp32
export ESPPORT=/dev/ttyUSB0
idf.py set-target esp32
idf.py -p "$ESPPORT" flash monitor
```

Verificacion:

```text
EDGE,t_ms=...,edge=0,zone=none,action=forward,...
EDGE,t_ms=...,edge=1,zone=left,action=turn_right,...
EDGE,t_ms=...,edge=1,zone=right,action=turn_left,...
EDGE,t_ms=...,edge=1,zone=center,action=reverse,...
```

Prueba acercando la camara al masking tape. Si detecta demasiado o muy poco,
ajusta con:

```bash
idf.py menuconfig
```

Ruta:

```text
Edge Detection Configuration
```

Luego recompila y flashea:

```bash
idf.py build
idf.py -p "$ESPPORT" flash monitor
```

## 6. Flashear ESP32-S3 controlador

Conecta solo el ESP32-S3 al computador.

```bash
cd ~/labs/Proyecto/habilitacion_dummy/code
get_esp32
export ESPPORT=/dev/ttyUSB0
idf.py set-target esp32s3
idf.py -p "$ESPPORT" flash monitor
```

Verificacion inicial:

```text
dummy_ctrl: Dummy qualification controller starting
dummy_ctrl: Reading identifier camera on UART1 RX GPIO17
dummy_ctrl: Reading edge camera on UART2 RX GPIO18
CTRL,mode=search,motion=left,...
```

Sin camaras conectadas, el S3 queda en `search` girando para buscar.

## 7. Cableado final sin computador

Apaga todo antes de cablear.

### UARTs de camaras hacia ESP32-S3

| Origen | Destino ESP32-S3 |
| --- | --- |
| TX de ESP32-CAM identificador | GPIO17 |
| TX de ESP32-CAM bordes | GPIO18 |
| GND de ambas ESP32-CAM | GND comun |

Solo se necesita TX de cada camara hacia RX del S3. No conectes TX del S3 a las
camaras para esta prueba.

### Motores L298N

| L298N | ESP32-S3 |
| --- | --- |
| ENA | GPIO15 |
| IN1 | GPIO4 |
| IN2 | GPIO5 |
| IN3 | GPIO6 |
| IN4 | GPIO7 |
| ENB | GPIO16 |
| GND | GND comun |

Importante:

- El GND de las camaras, ESP32-S3, L298N y bateria debe estar unido.
- No alimentes motores desde USB.
- Las camaras y el S3 deben recibir alimentacion estable.
- Para primera prueba, levanta el robot para que las ruedas no toquen el piso.

## 8. Prueba con monitor y robot levantado

Con el S3 conectado al computador y las camaras conectadas al S3:

```bash
cd ~/labs/Proyecto/habilitacion_dummy/code
get_esp32
export ESPPORT=/dev/ttyUSB0
idf.py -p "$ESPPORT" monitor
```

Verifica:

- Dummy a izquierda:

```text
CTRL,mode=align_left,motion=left,identifier=1,identifier_zone=left,...
```

- Dummy a derecha:

```text
CTRL,mode=align_right,motion=right,identifier=1,identifier_zone=right,...
```

- Dummy centrado:

```text
CTRL,mode=attack,motion=forward,identifier=1,identifier_zone=center,...
```

- Borde detectado:

```text
CTRL,mode=avoid_edge,...
```

## 9. Prueba en el ring

1. Deja el robot en el centro del ring.
2. Deja el dummy con identificador dentro del campo visual frontal.
3. Enciende el robot con bateria.
4. El robot debe girar hasta centrar el identificador.
5. Cuando `zone=center`, debe avanzar y empujar.
6. Si se acerca al masking tape, la camara de bordes debe forzar evasión.

Para cumplir habilitacion, la conducta esperada es:

```text
buscar -> centrar dummy -> avanzar -> empujar -> evadir borde si aparece
```

## 10. Ajustes rapidos

Si gira al lado contrario:

- Intercambia el sentido de motores en el cableado del L298N, o
- cambia `IN1/IN2` o `IN3/IN4` en `idf.py menuconfig`.

Si no empuja con fuerza:

```bash
cd ~/labs/Proyecto/habilitacion_dummy/code
get_esp32
idf.py menuconfig
```

Ruta:

```text
Dummy qualification controller -> Forward attack speed percent
```

Sube `Forward attack speed percent`, recompila y flashea:

```bash
idf.py build
idf.py -p "$ESPPORT" flash monitor
```

Si pierde el dummy:

- Agrega fotos ESP con el dummy a distintas distancias.
- Agrega fondos del ring reales.
- Reentrena con `training/train_identifier_full.py`.
- Verifica que `zone` cambia correctamente antes de probar con motores.

Si se sale del ring:

- Baja velocidad de ataque.
- Sube sensibilidad de borde en `cam_bordes`.
- Asegura que la camara de bordes apunte al piso frente al robot, no al frente.
