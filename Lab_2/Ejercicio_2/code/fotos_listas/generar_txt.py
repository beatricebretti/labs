import os

def generar_etiquetas():
    # El protocolo pide: nombre_archivo, presencia, cuadrante
    presencia = "1"
    lineas_etiquetas = []

    # Revisamos las carpetas 1, 2 y 3
    for cuadrante in ["1", "2", "3"]:
        if os.path.exists(cuadrante):
            archivos = os.listdir(cuadrante)
            for f in archivos:
                # Solo procesamos si es la imagen ROI (para saber el nombre original)
                if f.endswith("_ROI.jpg"):
                    nombre_original = f.replace("_ROI.jpg", ".jpg")
                    lineas_etiquetas.append(f"{nombre_original}, {presencia}, {cuadrante}")

    # Ordenamos por nombre de archivo para que quede celular_001, 002...
    lineas_etiquetas.sort()

    # Guardamos el archivo
    with open("etiquetas.txt", "w") as out:
        for linea in lineas_etiquetas:
            out.write(linea + "\n")
    
    print(f"¡Listo! Se generaron {len(lineas_etiquetas)} etiquetas ordenadas en etiquetas.txt")

if __name__ == "__main__":
    generar_etiquetas()