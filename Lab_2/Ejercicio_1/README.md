# Ejercicio 1
Aplicación web para controlar el vehículo del Lab 1 usando un ESP32-S3, L298N y 2 motores DC.

- Acceso WIFI local (hosteado por ESP32).
- Acceso a interfaz web `http://192.168.4.1 `.
- Slider de velocidad.

## Conexiones

### L298N a motores
- OUT1 / OUT2 -> motor izq
- OUT3 / OUT4 -> motor der

### L298N a ESP32
- ENA -> GPIO15
- IN1 -> GPIO4
- IN2 -> GPIO5
- IN3 -> GPIO6
- IN4 -> GPIO7
- ENB -> GPIO16
- GND -> GND on ESP32

### Batería
- L298N `+12V/Vs` -> external battery positivo
- L298N GND -> external battery negativo
- L298N GND -> ESP32 GND
- ESP32 by USB 

## Build y flash

```bash
source ~/.zprofile && source ~/.zshrc && get_esp32
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/<PORT> flash monitor
```

## Uso

1. Flash firmware
2. Conectar celular o pc al WIFI:
   - SSID: `ESP32-Autito`
   - Password: `autito123`
3. Open:

```text
http://192.168.4.1
```

4. Mantener una dirección para mover
5. Soltar para parar