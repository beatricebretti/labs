# Ejercicio 1

El objetivo de este ejercicio es validar la instalación del entorno ESP-IDF y la capacidad de modificar, compilar y flashear un programa básico en el ESP32-S3. Se modificó el ejemplo estándar "Hello World" para imprimir un mensaje personalizado.

- `main/hello_world_main.c`: Código fuente modificado para imprimir nombre.
- `output.pdf`: Documento con el screenshot de la salida serial.

## Requisitos
- ESP-IDF Instalado
* **Chip:** ESP32-S3.

## Instrucciones de Ejecución
Para compilar, flashear y monitorear este ejercicio, asegúrese de estar en la carpeta raíz del Ejercicio 1 y ejecute:
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-110 monitor
```
Nota: Reemplazar `/dev/cu.usbserial-110` por puerto específico si es diferente.