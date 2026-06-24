# Movimiento autonomo con comandos por microfono

Este firmware corre en el ESP32-S3 controlador del robot. Cumple la parte de
movimiento constante e interaccion por audio del enunciado:

- Al arrancar, el robot avanza solo. No necesita estar conectado al computador.
- Una palmada audible ordena un giro a la izquierda.
- Dos palmadas seguidas ordenan retirarse: retrocede y gira.
- No usa WiFi, BLE ni control remoto durante el combate.

El detector de audio es deliberadamente simple y robusto para demo: detecta
golpes sonoros cortos por un microfono analogico. Si quieren usar palabras como
`izquierda` o `retirate`, el siguiente paso seria entrenar un modelo de keyword
spotting; para cumplir la restriccion, las palmadas son instrucciones de audio
en rango audible humano.

## Hardware esperado

### ESP32-S3 a L298N

Estos pines son los mismos usados en el autito del laboratorio:

| Senal L298N | ESP32-S3 |
| --- | --- |
| ENA | GPIO15 |
| IN1 | GPIO4 |
| IN2 | GPIO5 |
| IN3 | GPIO6 |
| IN4 | GPIO7 |
| ENB | GPIO16 |
| GND | GND |

Conecta los motores a `OUT1/OUT2` y `OUT3/OUT4` del L298N.

Importante:

- La bateria de motores va al `+12V/Vs` del L298N y a `GND` del L298N.
- El `GND` de la bateria/L298N debe estar unido al `GND` del ESP32-S3.
- No alimentes motores desde el USB del computador.
- Para probar con el robot levantado, puedes alimentar el ESP por USB y el L298N
  con bateria.

### Microfono analogico

Usa un modulo con salida analogica, por ejemplo MAX4466, MAX9814 o KY-038 usando
pin `AO`.

| Microfono | ESP32-S3 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| OUT/AO | GPIO1 |

`GPIO1` se lee como `ADC1_CH0`. Si usas otro pin ADC, cambia
`CONFIG_AUDIO_MIC_ADC_CHANNEL` en `idf.py menuconfig`.

## Comandos en WSL

Entra al proyecto:

```bash
cd ~/labs/Proyecto/audio_motores/code
```

Activa ESP-IDF. En el computador del curso normalmente sirve:

```bash
get_esp32
```

Si no existe `get_esp32`, usa la ruta estandar:

```bash
source ~/esp/esp-idf/export.sh
```

Define el puerto:

```bash
export ESPPORT=/dev/ttyUSB0
echo "$ESPPORT"
```

Si no sabes el puerto:

```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Configura target una vez:

```bash
idf.py set-target esp32s3
```

Compila:

```bash
idf.py build
```

Flashea y monitorea para calibrar:

```bash
idf.py -p "$ESPPORT" flash monitor
```

Para salir del monitor:

```text
Ctrl+]
```

## Que deberias ver en monitor

Al iniciar:

```text
audio_motor: Audio ready: baseline=..., noise_peak=..., threshold=...
audio_motor: Motion=forward speed=65%
```

Da una palmada fuerte cerca del microfono. Deberias ver:

```text
AUDIO,command=turn_left,claps=1,...
```

Da dos palmadas separadas por menos de un segundo:

```text
AUDIO,command=retreat,claps=2,...
```

## Uso sin computador

Despues de flashear:

1. Desconecta el USB del computador.
2. Alimenta el ESP32-S3 con su fuente/bateria.
3. Alimenta el L298N con la bateria de motores.
4. Asegura GND comun entre ESP32-S3 y L298N.
5. Enciende el robot.

El firmware parte automaticamente en modo `forward`; no necesita terminal,
monitor serial, WiFi ni PC. Las instrucciones se dan con palmadas al microfono.

## Ajustar sensibilidad

Si no detecta palmadas, baja el umbral minimo:

```bash
idf.py menuconfig
```

Ruta:

```text
Robot sumo audio motor controller -> Minimum clap delta
```

Valores utiles:

- `180`: mas sensible, puede tomar ruido como palmada.
- `260`: default.
- `350`: menos sensible.

Luego:

```bash
idf.py build
idf.py -p "$ESPPORT" flash monitor
```

Si detecta falsos comandos por golpes del robot, sube el umbral o aleja el
microfono de los motores.

## Verificacion para la entrega

1. Levanta el robot para que las ruedas no toquen el piso.
2. Enciende con bateria: las ruedas deben avanzar solas.
3. Da una palmada: debe girar a la izquierda un momento y volver a avanzar.
4. Da dos palmadas: debe retroceder y girar, luego volver a avanzar.
5. Desconecta el computador: debe seguir funcionando igual con bateria.

