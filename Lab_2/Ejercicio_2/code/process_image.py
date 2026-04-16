import os
import argparse
from PIL import Image, ImageDraw, ImageOps

def procesar_imagen(ruta_entrada, ruta_base_salida):
    try:
        with Image.open(ruta_entrada) as img:
            # 1. CORRECCIÓN DE ROTACIÓN: Lee los metadatos y endereza la foto
            img = ImageOps.exif_transpose(img)
            
            # 2. Resize to 512x512 maintaining aspect ratio (padding with black bars)
            img = ImageOps.pad(img, (512, 512), color=0)
            
            # 3. Convert to grayscale (B&W)
            img = img.convert('L')
            
            # --- Save Clean Version (Training) ---
            img.save(ruta_base_salida)
            
            # --- Generate ROI Version ---
            draw = ImageDraw.Draw(img)
            alto = 512
            linea_1 = 170
            linea_2 = 341
            
            # Line between quadrant 1 and 2
            draw.line([(linea_1, 0), (linea_1, alto)], fill=255, width=2)
            # Line between quadrant 2 and 3
            draw.line([(linea_2, 0), (linea_2, alto)], fill=255, width=2)
            
            # Save ROI result
            base, ext = os.path.splitext(ruta_base_salida)
            ruta_roi_salida = f"{base}_ROI{ext}"
            img.save(ruta_roi_salida)

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