import sys
try:
    from PIL import Image
except ImportError:
    print("Please install Pillow: pip install Pillow")
    sys.exit(1)

def convert_image(image_path, output_header):
    # 1. Open the image and convert to 8-bit grayscale ('L')
    img = Image.open(image_path).convert('L')
    
    # 2. Resize to fit comfortably within ESP32 internal RAM (e.g., 128x128 = 16KB)
    # If your ESP32 has external PSRAM enabled, you can remove this or increase the size.
    img = img.resize((128, 128))
    width, height = img.size

    # 3. Write raw pixel values to a C array
    with open(output_header, 'w') as f:
        f.write('#pragma once\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'#define IMG_WIDTH {width}\n')
        f.write(f'#define IMG_HEIGHT {height}\n\n')
        f.write('const uint8_t se2_1_image[] = {\n')

        for y in range(height):
            for x in range(width):
                pixel = img.getpixel((x, y))
                f.write(f'{pixel}, ')
            f.write('\n')

        f.write('};\n')

    print(f"Successfully converted {image_path} to {output_header}")

if __name__ == '__main__':
    # Update this to your image's actual path if needed
    convert_image('SE2-1.jpg', 'main/se2_1_image.h')
