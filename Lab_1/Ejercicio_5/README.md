# Ejercicio 5

Este ejercicio integra el uso de actuadores (Motores DC mediante puente H L298N) y sensores analógicos (Micrófonos). El sistema detecta peaks de intensidad sonora y activa el movimiento.

- `main/main.c`: Lógica de lectura de ADC y control de GPIO para motores.
- `output.pdf`: Evidencia del funcionamiento y análisis.

## Conexiones (L298N + ESP32-S3)
* **Motores:** GPIO 4, 5, 6 y 7. (REVISAR)
* **Micrófonos:** GPIO 1 (Izq) y GPIO 2 (Der).
* **Alimentación:** Baterías externas al L298N, GND compartido con el ESP32.

## Instrucciones de Ejecución
Para compilar, flashear y monitorear este ejercicio, asegúrese de estar en la carpeta raíz del Ejercicio 5 y ejecute:
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-110 monitor
```
Nota: Reemplazar `/dev/cu.usbserial-110` por puerto específico si es diferente.