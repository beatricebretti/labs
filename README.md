# Laboratorios de Sistemas Embebidos

## Integrantes
- Simón Alonso
- Matias García-Huidobro
- Julián Pineda
- Beatrice Valdés

```text
           ______________________________
          |                              |
          |           ESP32-S3           |
          |        WiFi + BLE MCU        |
          |                              |
          |        ┌────────────┐        |
          |        │  ESP32-S3  │        |
          |        │   MODULE   │        |
          |        └────────────┘        |
          |                              |
 3V3  o---|                              |---o  GND
 EN   o---|                              |---o  GPIO1
 IO4  o---|                              |---o  GPIO2
 IO5  o---|                              |---o  GPIO3
 IO6  o---|                              |---o  GPIO4
 IO7  o---|                              |---o  GPIO5
 IO15 o---|                              |---o  GPIO6
 IO16 o---|                              |---o  GPIO7
          |                              |
          |         ┌────────┐           |
          |         │  USB   │           |
          |_________│ TYPE-C │___________|
```

## Estructura del Repositorio

Cada laboratorio tiene su propia carpeta con su respectivo código fuente, archivos de configuración y reportes de salida.
### [Laboratorio 1: MCU, sensores y actuadores](./Lab_1)
### [Laboratorio 2: Aplicación Web y AI](./Lab_2)