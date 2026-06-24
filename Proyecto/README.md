# Proyecto - cumplimiento de restricciones en WSL/Bash

Este directorio contiene tres firmwares ESP-IDF para demostrar la parte de
cumplimiento de restricciones del robot sumo:

- `cam_identificador/code`: firmware de una ESP32-CAM frontal/lateral que corre
  el modelo TinyML del identificador y emite telemetria `IDENTIFIER,...`.
- `cam_bordes/code`: firmware de una segunda ESP32-CAM apuntando al piso que
  detecta bordes del ring con Sobel y emite telemetria `EDGE,...`.
- `audio_motores/code`: firmware para el ESP32-S3 controlador. Mueve el robot
  autonomamente y acepta dos instrucciones por microfono: una palmada gira a la
  izquierda y dos palmadas ordenan retirarse.
- `habilitacion_dummy/code`: firmware para el ESP32-S3 que combina la telemetria
  del identificador y la camara de bordes para buscar, centrar y empujar el
  dummy sin salir del ring.
- `docs/`: protocolo de telemetria y checklist de lo que falta integrar en el
  ESP32-S3 controlador.
- `scripts/`: helpers Bash para revisar el entorno WSL y compilar ambos
  firmwares.
- `training/`: scripts para inspeccionar el dataset, entrenar el modelo del
  identificador, cuantizarlo a TFLite y reemplazar el arreglo C embebido.

La arquitectura propuesta es usar dos ESP32-CAM y un ESP32-S3 controlador. Las
camaras envian lineas por UART al S3. El S3 decide motores, procesa audio, mide
bateria y publica el dashboard web. Durante combate el robot no debe recibir
control por WiFi/BLE; WiFi queda solo para telemetria y las instrucciones de
conducta deben entrar por audio.

Para la parte de movimiento autonomo por microfono, usa la guia especifica en
`audio_motores/README.md`.

Para la habilitacion contra el robot dummy, usa la guia especifica en
`habilitacion_dummy/README.md`.

## Supuestos

Los comandos de esta guia estan escritos para Bash dentro de Ubuntu en WSL2.
Cuando un comando sea para PowerShell de Windows se marca explicitamente como
`PowerShell administrador`; todo lo demas va en la terminal WSL.

Se asume:

- Windows con WSL2 y Ubuntu instalado.
- ESP-IDF instalado en `~/esp/esp-idf`, o `IDF_PATH` apuntando a otra ruta.
- Placas ESP32-CAM AI-Thinker con PSRAM.
- Adaptador USB-serial visible en WSL como `/dev/ttyUSB0` o `/dev/ttyACM0`.
- El proyecto ubicado dentro del filesystem Linux de WSL, por ejemplo
  `~/se-labs/Proyecto`. Evita compilar desde `/mnt/c/...` porque suele ser mas
  lento y puede generar problemas de permisos.

Referencias oficiales utiles:

- ESP-IDF Get Started: <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html>
- ESP-IDF con WSL y `usbipd-win`: <https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/additionalfeatures/wsl.html>

## 1. Abrir WSL y confirmar que estas usando Bash

Ejecuta en Ubuntu/WSL:

```bash
echo "$SHELL"
echo "$BASH_VERSION"
uname -a
```

Por que: el README usa sintaxis Bash (`source`, variables y scripts `.sh`).
`uname -a` debe mencionar Linux y normalmente tambien `microsoft` o `WSL`.

Verificacion esperada:

- `echo "$SHELL"` muestra algo como `/bin/bash`.
- `echo "$BASH_VERSION"` imprime una version, por ejemplo `5.x`.
- `uname -a` confirma que estas dentro del kernel Linux de WSL.

Si tu shell no es Bash, entra a Bash con:

```bash
bash
```

## 2. Dejar el proyecto dentro del filesystem de Ubuntu

Si ya tienes `Proyecto` dentro de Ubuntu, entra directo:

```bash
cd ~/se-labs/Proyecto
pwd
ls
```

Por que: `cd` posiciona la terminal en la raiz del proyecto. `pwd` confirma la
ruta exacta y `ls` debe mostrar `cam_identificador`, `cam_bordes`, `docs` y
`scripts`.

Si el proyecto esta en Windows, copialo a Ubuntu una vez. Ajusta la ruta
`/mnt/c/Users/TU_USUARIO/...` a la ubicacion real:

```bash
mkdir -p ~/se-labs
cp -a /mnt/c/Users/TU_USUARIO/ruta/al/lab/Proyecto ~/se-labs/
cd ~/se-labs/Proyecto
pwd
ls
```

Verificacion esperada:

```text
cam_bordes  cam_identificador  docs  README.md  scripts
```

## 3. Instalar paquetes base de Ubuntu

Ejecuta:

```bash
sudo apt update
sudo apt install -y git wget flex bison gperf python3 python3-pip python3-venv python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util
```

Por que:

- `git` y `wget` permiten descargar ESP-IDF y componentes.
- `cmake`, `ninja-build` y `ccache` son usados por el sistema de build.
- `python3`, `pip`, `venv` y `setuptools` son usados por `idf.py`.
- `flex`, `bison`, `gperf`, `libffi-dev`, `libssl-dev` y `dfu-util` son
  dependencias normales del toolchain ESP-IDF en Linux.

Verificacion:

```bash
git --version
cmake --version
ninja --version
python3 --version
```

Cada comando debe imprimir una version sin error.

## 4. Instalar o activar ESP-IDF

Si ya tienes ESP-IDF instalado por el curso, no lo reinstales. Solo verifica la
ruta:

```bash
ls ~/esp/esp-idf/export.sh
```

Si ese archivo existe, activa el entorno:

```bash
source ~/esp/esp-idf/export.sh
idf.py --version
```

Por que: `export.sh` agrega `idf.py`, el compilador, Python virtualenv y
herramientas de ESP-IDF al `PATH` de esta terminal.

Verificacion esperada:

- `idf.py --version` imprime una version de ESP-IDF.
- Este proyecto requiere componentes compatibles con ESP-IDF `>=5.1`. Los locks
  actuales fueron generados con ESP-IDF `6.1.0`; si usas otra version compatible,
  ESP-IDF puede recalcular `dependencies.lock`.

Si no tienes ESP-IDF, instalalo en `~/esp/esp-idf`:

```bash
mkdir -p ~/esp
cd ~/esp
git clone -b release/v6.0 --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf
./install.sh esp32
source ~/esp/esp-idf/export.sh
idf.py --version
```

Por que:

- `git clone -b release/v6.0 --recursive` descarga ESP-IDF estable y sus
  submodulos.
- `./install.sh esp32` instala el toolchain necesario para placas ESP32.
- `source .../export.sh` activa ese toolchain en la terminal actual.

Verificacion adicional:

```bash
echo "$IDF_PATH"
command -v idf.py
```

`IDF_PATH` debe apuntar a `~/esp/esp-idf` y `command -v idf.py` debe mostrar una
ruta dentro de ESP-IDF.

## 5. Revisar el entorno del proyecto

Desde la raiz de `Proyecto`:

```bash
cd ~/se-labs/Proyecto
bash scripts/wsl_check_env.sh
```

Por que: el script revisa que estes en Bash/WSL, que exista `export.sh`, que
`idf.py` quede disponible, que ambos proyectos apunten al target `esp32`, que el
modulo de camara sea `AI_THINKER` y que WSL vea algun puerto serial.

Verificacion esperada:

```text
[OK] export.sh de ESP-IDF disponible
[OK] idf.py disponible: ESP-IDF ...
[OK] cam_identificador usa target esp32
[OK] cam_identificador usa modulo AI-Thinker
[OK] cam_bordes usa target esp32
[OK] cam_bordes usa modulo AI-Thinker
```

Si aparece `WARN` por no ver `/dev/ttyUSB*` ni `/dev/ttyACM*`, puedes compilar
igual. Ese puerto solo es necesario para flashear o monitorear una placa.

## 6. Entrenar o reemplazar el modelo del identificador

La ESP32-CAM no entrena el modelo. Solo ejecuta inferencia con el arreglo C que
esta en `cam_identificador/code/main/identifier_detect_model_data.cc`.

Antes de compilar el firmware final, revisa el dataset:

```bash
cd ~/se-labs/labs/Proyecto
python3 training/inspect_dataset.py --dataset ../../dataset_embebidos_grupo3
```

Verificacion actual esperada con el dataset del grupo:

```text
celular:
  presentes: 276
  ausentes: 1
esp:
  presentes: 193
  ausentes: 51
```

Con las fotos ESP actuales ya se puede entrenar el detector full-frame de cuatro
clases: `none`, `identifier_left`, `identifier_center` e `identifier_right`.
Esto permite que el robot use la foto completa y aun asi sepa hacia que lado
girar para centrar el dummy.

Entrena y reemplaza el modelo embebido:

```bash
cd ~/se-labs/labs/Proyecto
python3 -m venv .venv-train
source .venv-train/bin/activate
python -m pip install --upgrade pip
python -m pip install -r training/requirements.txt
python training/train_identifier_full.py \
  --dataset ../../dataset_embebidos_grupo3 \
  --sources esp \
  --epochs 80 \
  --emit-firmware
```

Por que:

- `inspect_dataset.py` valida etiquetas, balance de clases y archivos faltantes.
- `python -m venv .venv-train` separa TensorFlow del entorno de ESP-IDF.
- `--sources esp` entrena con imagenes parecidas a lo que vera la ESP32-CAM.
- `--emit-firmware` copia el `.tflite` convertido a los archivos C que compila
  ESP-IDF.

El firmware corre el modelo una vez sobre la imagen completa y publica
telemetria asi:

```text
IDENTIFIER,detected=1,zone=center,none_score=2,left_score=7,center_score=91,right_score=0,best_score=91,threshold=65
```

Los detalles completos estan en `training/README.md`.

## 7. Compilar la camara de identificador

Activa ESP-IDF y entra al firmware:

```bash
source ~/esp/esp-idf/export.sh
cd ~/se-labs/Proyecto/cam_identificador/code
```

Configura el target:

```bash
idf.py set-target esp32
```

Por que: ESP32-CAM AI-Thinker usa chip ESP32 clasico, no ESP32-S3. Este comando
asegura que el compilador, las opciones de SDK y los componentes sean para
`esp32`.

Verifica configuracion:

```bash
grep -E 'CONFIG_IDF_TARGET="esp32"|CONFIG_CAMERA_MODULE_AI_THINKER=y|CONFIG_SPIRAM=y' sdkconfig
```

Debe aparecer:

```text
CONFIG_IDF_TARGET="esp32"
CONFIG_CAMERA_MODULE_AI_THINKER=y
CONFIG_SPIRAM=y
```

Compila:

```bash
idf.py build
```

Por que: `idf.py build` descarga componentes declarados en `idf_component.yml`,
genera CMake/Ninja y produce el binario del firmware.

Verificacion:

```bash
test -f build/robot_sumo_identifier_camera.bin && echo "binario identificador OK"
```

Tambien puedes mirar el resumen de tamanos:

```bash
idf.py size
```

Salida de telemetria que debe emitir al correr:

```text
IDENTIFIER,detected=1,zone=center,none_score=2,left_score=7,center_score=91,right_score=0,best_score=91,threshold=65
```

Interpretacion:

- `detected=1`: el identificador fue detectado.
- `zone`: posicion estimada del identificador en la imagen completa.
- `none_score`: confianza de que no hay identificador.
- `left_score`, `center_score`, `right_score`: confianza por direccion.
- `best_score`: mayor score entre izquierda, centro y derecha.

## 8. Compilar la camara de bordes

Activa ESP-IDF y entra al firmware:

```bash
source ~/esp/esp-idf/export.sh
cd ~/se-labs/Proyecto/cam_bordes/code
```

Configura el target:

```bash
idf.py set-target esp32
```

Verifica configuracion base:

```bash
grep -E 'CONFIG_IDF_TARGET="esp32"|CONFIG_CAMERA_MODULE_AI_THINKER=y|CONFIG_SPIRAM=y|CONFIG_EDGE_' sdkconfig
```

Debe aparecer `esp32`, `AI_THINKER`, `SPIRAM` y parametros `CONFIG_EDGE_*`.

Compila:

```bash
idf.py build
```

Verificacion:

```bash
test -f build/robot_sumo_edge_camera.bin && echo "binario bordes OK"
idf.py size
```

Salida de telemetria que debe emitir al correr:

```text
EDGE,t_ms=12345,edge=1,zone=center,action=reverse,steer=0,confidence=100,distance_px=12,left=31,center=85,right=28,bottom=49,row_peak=18,thr=110
```

Interpretacion para el ESP32-S3:

- `edge=0`: mantener avance o estrategia actual.
- `zone=left`: girar a la derecha.
- `zone=right`: girar a la izquierda.
- `zone=center`: retroceder corto y luego girar.
- `action` es la accion sugerida para motores.
- `confidence`, `distance_px`, `left`, `center`, `right`, `bottom`,
  `row_peak` y `thr` sirven para calibrar.

## 9. Compilar ambos firmwares con un solo comando

Desde la raiz de `Proyecto`:

```bash
cd ~/se-labs/Proyecto
bash scripts/wsl_build_all.sh
```

Por que: el script activa ESP-IDF, entra a cada firmware, fija `set-target esp32`,
ejecuta `reconfigure` y luego `build`.

Verificacion esperada al final:

```text
[OK] Builds terminadas.
Binarios esperados:
  .../cam_identificador/code/build/robot_sumo_identifier_camera.bin
  .../cam_bordes/code/build/robot_sumo_edge_camera.bin
```

Confirma manualmente:

```bash
ls -lh cam_identificador/code/build/robot_sumo_identifier_camera.bin
ls -lh cam_bordes/code/build/robot_sumo_edge_camera.bin
ls -lh habilitacion_dummy/code/build/robot_sumo_dummy_qualification.bin
```

## 10. Conectar una ESP32-CAM a WSL para flashear

Primero conecta la placa a Windows por USB. Para ESP32-CAM normalmente usas un
adaptador USB-serial. Pon la placa en modo bootloader antes de flashear:

- `GPIO0` conectado a `GND`.
- `5V` y `GND` conectados.
- `U0R` de la ESP32-CAM al `TX` del adaptador.
- `U0T` de la ESP32-CAM al `RX` del adaptador.
- Presiona `RST` despues de conectar `GPIO0` a `GND`.

En `PowerShell administrador`, lista los dispositivos USB:

```powershell
usbipd list
```

Busca el `BUSID` del adaptador USB-serial, por ejemplo un CP210x o CH340.
Luego enlazalo una vez:

```powershell
usbipd bind --busid BUSID
```

Adjuntalo a WSL:

```powershell
usbipd attach --wsl --busid BUSID
```

Por que: Windows no expone automaticamente los puertos COM dentro de WSL.
`usbipd-win` permite que Ubuntu vea el adaptador como `/dev/ttyUSB0` o
`/dev/ttyACM0`.

Vuelve a WSL y verifica:

```bash
dmesg | tail
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Verificacion esperada:

- `dmesg` menciona `ttyUSB0` o `ttyACM0`.
- `ls` muestra un dispositivo como `/dev/ttyUSB0`.

Guarda el puerto en una variable para evitar errores al escribir:

```bash
export ESPPORT=/dev/ttyUSB0
test -e "$ESPPORT" && echo "puerto OK: $ESPPORT"
```

Si tu puerto es `/dev/ttyACM0`, usa:

```bash
export ESPPORT=/dev/ttyACM0
test -e "$ESPPORT" && echo "puerto OK: $ESPPORT"
```

## 11. Flashear y monitorear la camara de identificador

Con la ESP32-CAM del identificador conectada en modo bootloader:

```bash
source ~/esp/esp-idf/export.sh
cd ~/se-labs/Proyecto/cam_identificador/code
idf.py -p "$ESPPORT" flash
```

Por que: `flash` escribe el binario compilado en la memoria de la ESP32-CAM.

Verificacion esperada:

- El comando termina sin `Failed to connect`.
- Al final aparece `Hard resetting via RTS pin...` o un mensaje equivalente.

Para monitorear:

```bash
idf.py -p "$ESPPORT" monitor
```

Si la placa sigue en modo bootloader, desconecta `GPIO0` de `GND` y presiona
`RST`.

Verificacion de salida:

```text
IDENTIFIER,detected=0,zone=none,none_score=...,left_score=...,center_score=...,right_score=...
IDENTIFIER,detected=1,zone=center,none_score=...,left_score=...,center_score=...,right_score=...
```

Para salir del monitor presiona:

```text
Ctrl+]
```

## 12. Flashear y monitorear la camara de bordes

Con la segunda ESP32-CAM conectada en modo bootloader:

```bash
source ~/esp/esp-idf/export.sh
cd ~/se-labs/Proyecto/cam_bordes/code
idf.py -p "$ESPPORT" flash
```

Luego desconecta `GPIO0` de `GND`, presiona `RST` y abre el monitor:

```bash
idf.py -p "$ESPPORT" monitor
```

Verificacion de salida:

```text
EDGE,t_ms=...,edge=0,zone=none,action=forward,...
EDGE,t_ms=...,edge=1,zone=center,action=reverse,...
```

Prueba fisica minima:

- Apunta la camara al piso dentro del ring: deberia predominar `edge=0`.
- Acerca la camara al masking tape o borde: deberia aparecer `edge=1`.
- Si el borde aparece a la izquierda de la imagen, deberia sugerir
  `zone=left,action=turn_right`.
- Si aparece a la derecha, deberia sugerir `zone=right,action=turn_left`.
- Si aparece al centro y cerca, deberia sugerir `zone=center,action=reverse`.

## 13. Ajustar parametros de deteccion de bordes

Entra al menu de configuracion:

```bash
source ~/esp/esp-idf/export.sh
cd ~/se-labs/Proyecto/cam_bordes/code
idf.py menuconfig
```

Ruta dentro del menu:

```text
Application Configuration -> Edge Detection Configuration
```

Parametros principales:

- `Sobel gradient threshold`: subelo si hay falsos positivos por textura o
  sombras; bajalo si no detecta el masking tape.
- `Minimum edge hits`: subelo para exigir mas pixeles de borde; bajalo si el
  borde real aparece debil.
- `Bottom danger rows`: aumenta la franja inferior considerada peligrosa.
- `Danger distance px`: aumenta si quieres evadir antes.
- `Loop period ms`: baja el periodo para responder mas rapido, cuidando carga de
  CPU.

Despues de guardar:

```bash
idf.py build
idf.py -p "$ESPPORT" flash monitor
```

Verificacion:

- En la telemetria, `thr` debe reflejar el umbral configurado.
- Al acercar el borde, `confidence` debe subir y `distance_px` debe bajar.

## 14. Checklist de cumplimiento para la entrega

Antes de demostrar, revisa:

```bash
cd ~/se-labs/Proyecto
bash scripts/wsl_check_env.sh
ls -lh cam_identificador/code/build/robot_sumo_identifier_camera.bin
ls -lh cam_bordes/code/build/robot_sumo_edge_camera.bin
```

Checklist funcional:

- La camara de identificador imprime `IDENTIFIER,...`.
- La camara de bordes imprime `EDGE,...`.
- El ESP32-S3 de `habilitacion_dummy` recibe ambas UART y parsea por prefijo.
- El S3 convierte `EDGE action` en motores y centra el dummy con `IDENTIFIER zone`.
- El robot se mueve al menos 10 cm cada 5 s si no hay contacto directo.
- El robot evade antes de cruzar el masking tape.
- El identificador geometrico mide mas de 5 cm y es visible en 360 grados.
- Solo se usan camaras y microfonos como sensores.
- WiFi/BLE no recibe control remoto durante combate.
- Audio reconoce al menos dos instrucciones.
- El dashboard web publica bordes, bateria, velocidad y robots detectados.

## 15. Problemas comunes

Si `idf.py` no existe:

```bash
source ~/esp/esp-idf/export.sh
command -v idf.py
```

Si WSL no ve la placa:

```bash
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
dmesg | tail
```

Luego repite en `PowerShell administrador`:

```powershell
usbipd list
usbipd attach --wsl --busid BUSID
```

Si `flash` falla conectando:

- Revisa que `GPIO0` este a `GND`.
- Presiona `RST` justo antes o durante `Connecting...`.
- Revisa que `ESPPORT` apunte al puerto correcto.

```bash
echo "$ESPPORT"
test -e "$ESPPORT" && echo "puerto existe"
```

Si la camara falla al iniciar:

- Confirma que la placa sea AI-Thinker.
- Confirma `CONFIG_CAMERA_MODULE_AI_THINKER=y`.
- Confirma alimentacion estable de 5V; la ESP32-CAM suele fallar con corriente
  insuficiente.

```bash
grep CONFIG_CAMERA_MODULE_AI_THINKER sdkconfig
grep CONFIG_SPIRAM sdkconfig | head
```

Si el detector de bordes marca demasiado:

```bash
cd ~/se-labs/Proyecto/cam_bordes/code
idf.py menuconfig
```

Sube `Sobel gradient threshold` o `Minimum edge hits`, guarda, recompila y
flashea.
