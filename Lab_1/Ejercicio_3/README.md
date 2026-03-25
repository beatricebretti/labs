# Lab 1 - Ejercicio 3

Este ejercicio implementa el pseudocódigo pedido en la sección 2.2 Ejercicio 3 del laboratorio.

## Estructura
- `main/main.c`: código principal del benchmark
- `output.pdf`: gráficos y análisis pedidos
- `sdkconfig.defaults`: configuración base sugerida

## Requisitos
- ESP-IDF instalado
- ESP32 conectado
- Puerto serial correcto

## Compilar, flashear y monitorear
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-1120 monitor