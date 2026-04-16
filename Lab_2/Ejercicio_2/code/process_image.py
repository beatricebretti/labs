import os
import argparse
from PIL import Image, ImageDraw, ImageOps

def procesar_imagen(ruta_entrada, ruta_base_salida):
    try:
        with Image.open(ruta_entrada) as img:
            # 1. CORRECCIÓN DE ROTACIÓN
            img = ImageOps.exif_transpose(img)
            
            # 2. CAMBIO CRÍTICO: Usar 'fit' en lugar de 'pad'
            # 'fit' recorta la imagen para que llene exactamente 512x512 sin bordes negros.
            img = ImageOps.fit(img, (512, 512), centering=(0.5, 0.5))
            
            # 3. Convertir a escala de grises
            img = img.convert('L')
            
            # --- Guardar versión limpia ---
            img.save(ruta_base_salida)
            
            # --- Generar versión ROI con cuadrantes ---
            draw = ImageDraw.Draw(img)
            linea_1, linea_2 = 170, 341
            draw.line([(linea_1, 0), (linea_1, 512)], fill=255, width=2)
            draw.line([(linea_2, 0), (linea_2, 512)], fill=255, width=2)
            
            base, ext = os.path.splitext(ruta_base_salida)
            img.save(f"{base}_ROI{ext}")

    except Exception as e:
        print(f"Error procesando {ruta_entrada}: {e}")

def procesar_directorio(input_dir, output_dir):
    imagenes_soportadas = ('.jpg', '.jpeg', '.png')
    
    # Obtener lista de archivos y ordenarlos para que el número sea consistente
    archivos = sorted([f for f in os.listdir(input_dir) if f.lower().endswith(imagenes_soportadas)])
    
    print(f"Procesando {len(archivos)} imágenes...")
    
    for i, nombre_archivo in enumerate(archivos, start=1):
        ruta_abs_entrada = os.path.join(input_dir, nombre_archivo)
        
        # NUEVO NOMBRE: "celular_001.jpg", "celular_002.jpg", etc.
        nuevo_nombre = f"celular_{i:03d}.jpg"
        ruta_abs_salida = os.path.join(output_dir, nuevo_nombre)
        
        procesar_imagen(ruta_abs_entrada, ruta_abs_salida)
        if i % 50 == 0:
            print(f"Progreso: {i}/{len(archivos)} imágenes listas.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Procesa y renombra imágenes para el dataset.")
    parser.add_argument("--input", "-i", type=str, required=True, help="Carpeta de entrada")
    parser.add_argument("--output", "-o", type=str, required=True, help="Carpeta de salida")
    
    args = parser.parse_args()
    
    if not os.path.isdir(args.input):
        print(f"Error: La carpeta '{args.input}' no existe.")
    else:
        os.makedirs(args.output, exist_ok=True)
        procesar_directorio(args.input, args.output)
        print("-" * 40)
        print("¡Listo! Imágenes derechas y renombradas en la carpeta de salida.")