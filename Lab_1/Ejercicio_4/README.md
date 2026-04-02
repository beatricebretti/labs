# Ejercicio 4

Este ejercicio mide el costo de acceso a distintas memorias del ESP32/ESP32-S3
usando una operación de multiplicación vector-escalar.

## Qué hace
Para cada tipo de memoria:
- inicializa datos
- ejecuta `result[i] = vector[i] * num`
- mide ciclos con `esp_cpu_get_cycle_count()`
- calcula ciclos por byte accedido
- imprime resultados en formato CSV

## Memorias evaluadas
- Static DRAM
- Static IRAM
- Static RTC
- Static FLASH (lectura desde rodata, escritura a DRAM)
- Dynamic DRAM
- Dynamic IRAM
- Dynamic PSRAM

## Requisitos
- ESP-IDF instalado
- placa con PSRAM si se quiere medir PSRAM
- PSRAM habilitada en `idf.py menuconfig`

## Instrucciones de Ejecución
Para compilar, flashear y monitorear este ejercicio, asegúrese de estar en la carpeta raíz del Ejercicio 4 y ejecute:
```bash
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/cu.usbserial-110 monitor
```
Nota: Reemplazar `/dev/cu.usbserial-110` por puerto específico si es diferente.