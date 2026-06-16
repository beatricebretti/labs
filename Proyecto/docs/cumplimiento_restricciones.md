# Checklist de cumplimiento

## Para demostrar el 15/22 de junio

- Movimiento constante: el S3 debe mover al menos 10 cm cada 5 s si no hay
  contacto directo.
- No salir del ring: la camara de bordes debe activar evasion antes de cruzar el
  masking tape.
- Identificacion geometrica visible: mantener el cilindro sobre 5 cm y visible
  en 360 grados.
- Sensores permitidos: solo camaras y microfonos.
- Comunicacion externa: no recibir control remoto por WiFi/BLE; usar audio para
  instrucciones.
- Audio: implementar al menos dos comandos, por ejemplo `gira izquierda` y
  `retirate`.
- Web: publicar telemetria de bordes, bateria, velocidad y robots detectados.
- ML local: mantener el modelo TinyML del identificador corriendo en la
  ESP32-CAM.

## Tareas pendientes fuera de este directorio

- Integrar las dos UART al ESP32-S3 controlador.
- Conectar la salida `action` de bordes a la logica de motores.
- Agregar lectura de bateria y estimacion de velocidad.
- Implementar reconocimiento de dos instrucciones de audio en el S3.
- Publicar la telemetria agregada en el dashboard web.
