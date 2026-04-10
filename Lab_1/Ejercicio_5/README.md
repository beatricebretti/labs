# Ejercicio_5 

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
- +Vmotor -> batería de motores 

### MAX9814
- VDD -> 3V3 ESP32
- GND -> GND ESP32
- OUT -> GPIO1 (ADC1_CH0)

## Frecuencias de comando por defecto
- 650 Hz -> Adelante
- 900 Hz -> Atrás
- 1150 Hz -> Izquierda
- 1400 Hz -> Derecha

## Parámetros para el análisis
- `SAMPLE_RATE_HZ`
- `FFT_SIZE`
- `ADC_WIDTH_USED`

## Compilar y flashear
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/<PORT> flash monitor
```

