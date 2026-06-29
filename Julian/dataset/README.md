# Dataset de latas

Coloca las imagenes tomadas con la ESP32-CAM en estas carpetas:

```text
dataset/
  latas/      # 75 fotos donde aparece una lata
  no_latas/   # 75 fotos de otras cosas
```

Pueden ser JPG de 320x240. El entrenamiento las convierte a escala de grises y
las redimensiona a 96x96 para que calcen con el firmware de la ESP32-CAM.
