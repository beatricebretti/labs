# Captura de dataset con ESP32-CAM

Esta carpeta sirve para tomar fotos nuevas con la ESP32-CAM y agregarlas al
dataset `dataset_embebidos_grupo3/esp`.

El flujo tiene dos partes:

- `firmware/`: programa temporal para la ESP32-CAM. Captura una imagen en gris
  cada 2 segundos y la envia por UART en base64.
- `capture_dataset.py`: script para WSL/PC. Lee esas imagenes, las convierte a
  JPG, las guarda en el dataset y agrega la etiqueta.

## 1. Flashear firmware de captura

Desde WSL:

```bash
cd ~/labs/Proyecto/dataset_capture/firmware
source ~/esp/esp-idf/export.sh
idf.py -p "$ESPPORT" flash
```

Este firmware no corre el modelo. Solo toma fotos para dataset.

## 2. Preparar Python

En otra terminal de WSL:

```bash
cd ~/labs/Proyecto/dataset_capture
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## 3. Sacar fotos negativas

Quita el identificador de la vista de la camara y ejecuta:

```bash
python capture_dataset.py --port "$ESPPORT" --label none --count 50
```

Eso guarda 50 fotos nuevas, por defecto, en:

```text
../../../dataset_embebidos_grupo3/esp
```

Cada foto queda separada por al menos 2 segundos porque el firmware captura con
ese intervalo.

## 4. Sacar fotos por la derecha

Pon el identificador en la zona derecha de la imagen y ejecuta:

```bash
python capture_dataset.py --port "$ESPPORT" --label right --count 40
```

Tambien puedes usar:

```bash
python capture_dataset.py --port "$ESPPORT" --label left --count 20
python capture_dataset.py --port "$ESPPORT" --label center --count 20
```

## 5. Verificar etiquetas

Despues de capturar:

```bash
cd ~/labs/Proyecto
python training/inspect_dataset.py --dataset ../../dataset_embebidos_grupo3 --sources esp
```

Ademas, esta carpeta mantiene un archivo de clases:

```text
../../../dataset_embebidos_grupo3/esp/etiquetas_clases.txt
```

Formato:

```text
esp_001.jpg, center
esp_010.jpg, none
esp_146.jpg, right
```

Ese archivo se usara para el modelo nuevo de 4 clases: `none`, `left`,
`center`, `right`.

## 6. Volver al firmware del detector

Cuando termines de capturar fotos, vuelve a flashear el detector normal:

```bash
cd ~/labs/Proyecto/cam_identificador/code
source ~/esp/esp-idf/export.sh
idf.py -p "$ESPPORT" flash monitor
```
