# Ejercicio 4 — Detección del identificador

Ambos entregables estan en output.pdf, de todas formas aca el video: [LINK](https://drive.google.com/file/d/1ArEwRSzRqrnC5P5_ICRhteBGF7fTvK4H/view?usp=share_link)


## Paso 1 — Imágenes estáticas 

```bash
cd labs/Lab_3/Ejercicio_4/images
python prepare_static_images.py
```

1. `CLI_ONLY_INFERENCE` `1` en `code/main/esp_main.h` (modo imágenes estáticas).
2. Build, flash y abre el monitor — **clasifica automáticamente** image0–9 al iniciar:

```bash
cd ../code
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

No hace falta escribir comandos en la consola. Repite el ciclo cada ~5s (se hardcodeo las imagenes ya que no funcionaba typear en consola en mac)

| Imagen ESP | Carpeta origen | Clase esperada |
|------------|----------------|----------------|
| image0–4 | `images/con_identificador/` | identifier |
| image5–9 | `images/sin_identificador/` | no identifier |

## Paso 2 — Cámara en vivo

1. En `code/main/esp_main.h`:

```c
#define CLI_ONLY_INFERENCE 0
```

2. Build, flash y abre el monitor serial en la terminal:

```bash
cd ../code
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

3. Apunta la ESP32-CAM al identificador. En la terminal verás:
   - Cada frame: `identifier score:XX%, no_identifier score:YY%`
   - Al detectar: `IDENTIFIER DETECTED! (score: XX%)`
