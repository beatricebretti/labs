# Ejercicio 8
El objetivo es mejorar el contraste de la imagen capturada por el sensor mediante la ecualización de su histograma en escala de grises.

- `main/take_picture.c`: Código fuente configurado para capturar fotos cada 5 segundos.
- `output.pdf`: Documento con los resultados visuales comparativos.

| Imagen Original | Imagen Ecualizada |  
| :---:| :---:|
|![PNG](https://github.com/beatricebretti/labs/blob/main/Lab_1/Ejercicio_7/ejercicio7.png)|![PNG](https://github.com/beatricebretti/labs/blob/main/Lab_1/Ejercicio_8/ejercicio8.png) |

## Requisitos
- ESP-IDF Instalado
- Placa ESP32-CAM (AI-Thinker)

## Instrucciones de Ejecución
Para compilar, flashear y monitorear este ejercicio, asegúrese de estar en la carpeta raíz del Ejercicio 8 y ejecute:
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-110 monitor
```
Nota: Reemplazar `/dev/cu.usbserial-110` por puerto específico si es diferente.


