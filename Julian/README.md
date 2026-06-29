# Julian - detector de latas en ESP32-CAM

Este directorio contiene lo necesario para entrenar y desplegar un detector
binario de latas:

- `dataset/`: coloca aqui tus imagenes JPG de la ESP32-CAM.
- `training/`: script de entrenamiento, conversion a TFLite INT8 y generacion
  del arreglo C.
- `cam_latas/code`: firmware ESP-IDF para ESP32-CAM AI-Thinker.

El firmware espera imagenes en escala de grises de `96x96`. Tus fotos pueden ser
JPG `320x240`; el script de entrenamiento las redimensiona automaticamente.

## 1. Preparar dataset

Coloca las 150 imagenes asi:

```text
Julian/dataset/
  latas/      # 75 fotos con lata
  no_latas/   # 75 fotos sin lata
```

La clase positiva es `can`; la clase negativa es `no_can`.

## 2. Entrenar y reemplazar el modelo del firmware

Desde la raiz de `labs`:

```bash
cd Julian
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r training/requirements.txt
python training/train_can_detector.py --dataset dataset --epochs 80 --emit-firmware
```

Si `tensorflow` no instala con tu `python3`, usa Python 3.11/3.12 o WSL con un
entorno Python compatible. El firmware ESP-IDF no depende de TensorFlow en el
computador; TensorFlow solo se usa para entrenar y generar el arreglo C.

El comando genera:

```text
training/out/can_model.keras
training/out/can_detect_model.tflite
training/out/can_detect_model_data.cc
training/out/can_detect_model_data.h
training/out/metrics.json
```

Con `--emit-firmware`, tambien reemplaza:

```text
cam_latas/code/main/can_detect_model_data.cc
cam_latas/code/main/can_detect_model_data.h
```

Importante: el modelo que viene inicialmente en el firmware es solo un
placeholder copiado desde el proyecto anterior. Para detectar latas de verdad,
primero hay que ejecutar el entrenamiento con tus imagenes.

## 3. Compilar y flashear

```bash
cd cam_latas/code
source /Users/matiasghf/esp/idf/esp-idf/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

Cambia `/dev/cu.usbserial-XXXX` por el puerto real de tu adaptador USB-serial.

## 4. Salida esperada

En el monitor serial deberias ver lineas parseables como:

```text
CAN,detected=1,can_score=87,no_can_score=13,threshold=65
CAN,detected=0,can_score=22,no_can_score=78,threshold=65
```

La decision actual usa umbral fijo de `65%` en
`cam_latas/code/main/detection_responder.cc`.

## 5. Que modelo se usa

Es una CNN pequena cuantizada a INT8:

```text
96x96x1
-> Conv2D(8)
-> AveragePooling
-> Conv2D(16)
-> AveragePooling
-> Conv2D(24)
-> AveragePooling
-> Dense(24)
-> Dense(2)
-> Softmax
```

Se usa esta arquitectura porque es suficientemente liviana para ESP32-CAM con
PSRAM y porque una lata puede variar por iluminacion, escala y fondo. Sobel o
reglas de color serian mas fragiles para este caso.
