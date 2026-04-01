# Ejercicio 7
- `main/take_picture.c`: Código fuente configurado para capturar fotos cada 5 segundos.
- `ejercicio7.png`: Imagen final reconstruida a partir de los datos hexadecimales.
- `link.txt`: Enlace al Google Colab utilizado para la visualización.

Se incrementó el `Task Watchdog timeout period` a 30 segundos para permitir que el bucle de impresión de datos hexadecimales termine sin reiniciar el chip.

Estructura `camera_config_t` se configuró con:
- `PIXFORMAT_GRAYSCALE`
- `FRAMESIZE_96X96`
- `CAMERA_FB_IN_DRAM`