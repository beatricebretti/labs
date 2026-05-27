from PIL import Image
import numpy as np
import sys

img = Image.open(sys.argv[1]).convert("L")
img = img.resize((96,96))

arr = np.array(img, dtype=np.uint8)

with open(sys.argv[2], "wb") as f:
    f.write(arr.tobytes())

print("done")