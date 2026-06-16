# Protocolo de telemetria

Las camaras emiten una linea por frame en formato CSV con claves `clave=valor`.
El ESP32-S3 puede leer cada UART hasta `\n`, revisar el prefijo y actualizar el
estado global del robot.

## Identificador

```text
IDENTIFIER,detected=0|1,identifier_score=0..100,no_identifier_score=0..100
```

Campos:

- `detected`: `1` si el modelo TinyML detecta el identificador.
- `identifier_score`: confianza de la clase identificador.
- `no_identifier_score`: confianza de la clase sin identificador.

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
