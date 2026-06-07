# Ejercicio 3 — Integrando la cámara 

Detección de personas en tiempo real con ESP32-CAM (módulo AI-Thinker).

- `CLI_ONLY_INFERENCE 0` en `main/esp_main.h` 
- `CONFIG_CAMERA_MODULE_AI_THINKER=y` en `sdkconfig.defaults`
- Salida serial en terminal (`idf.py monitor`) cuando se detecta una persona

## Hardware

- **Placa:** ESP32-CAM AI-Thinker (con PSRAM)

## Build y flash

```bash
cd labs/Lab_3/Ejercicio_3/code

idf.py set-target esp32


idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```


1. Al iniciar: `Camera Initialized`
2. En cada frame: `person score:XX%, no person score YY%`
3. Cuando hay persona (score ≥ 60%): aparece `PERSON DETECTED!` en la terminal (`idf.py monitor`)


