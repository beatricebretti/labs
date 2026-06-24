# Protocolo de telemetria

Las camaras emiten una linea por frame en formato CSV con claves `clave=valor`.
El ESP32-S3 puede leer cada UART hasta `\n`, revisar el prefijo y actualizar el
estado global del robot.

## Identificador

```text
IDENTIFIER,detected=0|1,zone=none|left|center|right,none_score=0..100,left_score=0..100,center_score=0..100,right_score=0..100,best_score=0..100,threshold=0..100
```

Campos:

- `detected`: `1` si el modelo TinyML detecta el identificador.
- `zone`: zona donde el modelo estima que esta el identificador. `none` indica
  que no hay deteccion confiable.
- `none_score`: confianza de la clase sin identificador.
- `left_score`, `center_score`, `right_score`: confianza de las clases de
  identificador por posicion usando la imagen completa.
- `best_score`: mayor confianza entre `left`, `center` y `right`.
- `threshold`: umbral minimo usado por el firmware para aceptar deteccion.

## Bordes

```text
EDGE,t_ms=...,edge=0|1,zone=none|left|center|right,action=forward|turn_left|turn_right|reverse,steer=-80|0|80,confidence=0..100,distance_px=...,left=...,center=...,right=...,bottom=...,row_peak=...,thr=...
```

Campos principales:

- `edge`: `1` cuando el borde esta suficientemente cerca para evadir.
- `zone`: sector de la imagen donde aparece el borde.
- `action`: accion recomendada para motores.
- `steer`: correccion lateral sugerida. Negativo gira a izquierda, positivo a
  derecha.
- `confidence`: confianza heuristica basada en pixeles con gradiente fuerte.
- `distance_px`: distancia aproximada desde el borde inferior de la imagen a la
  fila con mas borde. Menor valor implica mayor riesgo.

Campos de calibracion:

- `left`, `center`, `right`: cantidad de pixeles de borde por tercio de imagen.
- `bottom`: pixeles de borde en la franja inferior de peligro.
- `row_peak`: maximo de pixeles de borde en una fila.
- `thr`: umbral Sobel usado.
