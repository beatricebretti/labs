# Ejercicio 7
- `main/take_picture.c`: Código fuente configurado para capturar fotos cada 5 segundos.
- `output.pdf`: Documento con la imagen reconstruida del grupo.

| Imagen Capturada (96x96) |
| :---: |
| ![PNG](https://github.com/beatricebretti/labs/blob/main/Lab_1/Ejercicio_7/ejercicio7.png) |

## Requisitos
- ESP-IDF Instalado
- Placa ESP32-CAM (AI-Thinker)

## Instrucciones de Ejecución
Para compilar, flashear y monitorear este ejercicio, asegúrese de estar en la carpeta raíz del Ejercicio 7 y ejecute:
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-110 monitor
```
Nota: Reemplazar `/dev/cu.usbserial-110` por puerto específico si es diferente.

  
