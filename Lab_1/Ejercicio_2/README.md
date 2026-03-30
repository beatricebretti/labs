# Ejercicio 2

Se implementa un algoritmo de detección de bordes utilizando el Operador Sobel en un ESP32. El flujo de trabajo consiste en pre procesar una imagen en Python, convertirla a una matriz de bytes en C, y procesarla en el microcontrolador.

- `convert_image.py`: Script de Python para redimensionar y convertir imágenes a arreglos de C.
- `link.txt`: Enlace al Google Colab utilizado para la visualización de resultados.
- `img1.jpeg`: imagen de entrada
- `img2.jpeg`: imagen de salida

## Instalación y ejecución final
````
pip install Pillow
python convert_image.py

source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/<PORT> monitor