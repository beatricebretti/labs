# Proyecto - Cumplimiento de restricciones

Primera version del software para demostrar cumplimiento de restricciones del
robot sumo:

- `cam_identificador/code`: firmware ESP-IDF copiado desde `Lab_3/Ejercicio_4`
  para detectar el identificador con TinyML.
- `cam_bordes/code`: firmware nuevo para una segunda ESP32-CAM apuntando hacia
  el piso. Detecta bordes del ring con filtros Sobel y emite una accion de
  evasion por serial.
- `docs/`: protocolo de telemetria y checklist de la entrega.

## Arquitectura propuesta

Usar una ESP32-CAM mirando al frente/lateral para el identificador y una segunda
ESP32-CAM inclinada hacia abajo para el borde. Ambas placas envian lineas por
UART al ESP32-S3 controlador. El S3 decide motores, procesa audio, mide bateria
y publica el dashboard web.

El robot no debe recibir control por WiFi/BLE durante combate. La salida por
WiFi al dashboard debe ser solo telemetria; las instrucciones de conducta deben
entrar por audio.

## Camara de identificador

No necesitas conectar la placa para compilar. Solo conectala cuando vayas a
flashear o mirar el monitor serial.

```bash
cd Proyecto/cam_identificador/code
source /Users/matiasghf/esp/idf/esp-idf/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

Salida parseable:

```text
IDENTIFIER,detected=1,identifier_score=92,no_identifier_score=8
```

## Camara de bordes

No necesitas conectar la placa para compilar. Solo conectala cuando vayas a
flashear o mirar el monitor serial.

```bash
cd Proyecto/cam_bordes/code
source /Users/matiasghf/esp/idf/esp-idf/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

Salida parseable:

```text
EDGE,t_ms=12345,edge=1,zone=center,action=reverse,steer=0,confidence=100,distance_px=12,left=31,center=85,right=28,bottom=49,row_peak=18,thr=110
```

Regla de control sugerida en el S3:

- `edge=0`: mantener avance o estrategia actual.
- `zone=left`: girar a la derecha.
- `zone=right`: girar a la izquierda.
- `zone=center`: retroceder corto y luego girar.

Los parametros de deteccion se ajustan con `idf.py menuconfig` en
`Application Configuration -> Edge Detection Configuration`.
