# Ejercicio 4

Implementación y los resultados del benchmark de rendimiento. El objetivo es proyectar el comportamiento de 5 redes neuronales distintas en una **ESP32-S3 N16R8**, midiendo latencia, uso de memoria y consumo energético.

## Archivos del ejercicio
- `code/` :Código fuente completo (ESP-IDF)
- `output.pdf` : Informe.

## Instrucciones de menuconfig
- Falsh size 16MB
- PSRAM SPI connected RAM, Octal Mode PSRAM

## Build y flash

```bash
source ~/.zprofile && source ~/.zshrc && get_esp32
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/<PORT> flash monitor
```