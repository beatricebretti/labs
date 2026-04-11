# Ejercicio 5 

Implementación de un sistema de control por audio para un vehículo robótico utilizando el ESP32-S3. 

[LINK VIDEO](https://drive.google.com/file/d/1Nhei9vwUxPeMDaPP505f2WacGgvNZf9K/view?usp=sharing)

El microcontrolador captura la señal de un micrófono, realiza una FFT y detecta uno de cuatro tonos de audio para ejecutar los comandos de movimiento:

## Frecuencias de comandos
- 650 Hz -> Adelante
- 900 Hz -> Atrás
- 1150 Hz -> Izquierda
- 1400 Hz -> Derecha

Además, se realizó un análisis experimental variando `SAMPLE_RATE_HZ`, `FFT_SIZE` y `ADC_WIDTH_USED` para evaluar su efecto sobre resolución en frecuencia, estabilidad de detección y latencia en: `output.pdf`

## Hardware usado
- ESP32-S3 N8R2
- 2x motores TT DC
- L298N
- MAX9814 

## Pines usados
### L298N
- ENA -> GPIO15
- IN1 -> GPIO4
- IN2 -> GPIO5
- IN3 -> GPIO6
- IN4 -> GPIO7
- ENB -> GPIO16
- GND -> GND ESP32
- +Vmotor -> batería externa 

### MAX9814
- VDD -> 3V3 ESP32
- GND -> GND ESP32
- OUT -> GPIO1 (ADC1_CH0)



## Parámetros para el análisis
- `SAMPLE_RATE_HZ` (4000 / 8000 / 12000 Hz)
- `FFT_SIZE` (256 / 512 / 1024)
- `ADC_WIDTH_USED` (9 / 10 / 11 / 12 bits)

## Archivos del ejercicio
- `code/` → Código fuente completo (ESP-IDF)
- `output.pdf` → Informe completo con análisis experimental, espectros y conclusiones
- `video.txt` → Enlace del video de demostración

## Instrucciones de Ejecución
Para compilar, flashear y monitorear este ejercicio, asegúrese de estar en la carpeta raíz del Ejercicio 5 y ejecute:
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-110 monitor
```
Nota: Reemplazar `/dev/cu.usbserial-110` por puerto específico si es diferente.

