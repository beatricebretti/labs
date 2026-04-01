# Ejercicio 2

En este ejercicio se procesó una imagen monocromática utilizando un filtro de detección de bordes (Operador Sobel). La imagen original fue pre-procesada en Google Colab para ajustarla a una resolución de 96x96 píxeles y convertida a un arreglo de valores de 8 bits (grayscale).

- `main/ejercicio_2.c`: Implementación de los kernels de Sobel (Gx, Gy) y cálculo de magnitud.
- `main/se2_1_image.h`: Imagen de entrada en formato de arreglo C.
- `output.pdf`: Comparativa de la imagen de entrada y la salida procesada.
- `link.txt`: Enlace a Google Colab.

## Requisitos
- ESP-IDF Instalado
* **Chip:** ESP32-S3.

## Instrucciones de Ejecución
Para compilar, flashear y monitorear este ejercicio, asegúrese de estar en la carpeta raíz del Ejercicio 1 y ejecute:
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-110 monitor
```
Nota: Reemplazar `/dev/cu.usbserial-110` por puerto específico si es diferente.