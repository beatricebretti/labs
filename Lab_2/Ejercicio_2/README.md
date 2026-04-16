# Ejercicio 2

Se generó el dataset de entrenamiento.

## [Repositorio con Dataset](https://github.com/beatricebretti/dataset_embebidos_grupo3/tree/main)

## [Drive con imágenes](https://drive.google.com/drive/folders/1j9X3sroeNQrFQOqaNb0oNcutAbRP88kQ?usp=sharing)

Se utilizaron dos herramientas de procesamiento desarrolladas en Python:

- [`process_image.py`](./code/process_image.py): Toma las fotos de la carpeta `fotos_originales/`, aplica corrección de rotación automática, realiza un re escalado proporcional y recorte (fit) para obtener imágenes de 512x512 sin deformación, convierte las imágenes a escala de grises y genera una versión temporal _ROI con guías visuales para facilitar el etiquetado manual por cuadrantes.
- [`generar_txt.py`](./code/generar_txt.py): Automatiza la creación del archivo `etiquetas.txt`. Lee la organización de las carpetas temporales (`1/`, `2/`, `3/`) donde se clasificaron las fotos según su posición y genera el formato: `nombre_archivo, presencia, cuadrante`.