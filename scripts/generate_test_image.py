#!/usr/bin/env python3
import numpy as np

# Create a 1920x1080 test image with geometric patterns
width = 1920
height = 1080

# Create blank image
img = np.zeros((height, width), dtype=np.uint8)

# Add some geometric patterns for testing
# Center circle
center_x, center_y = width // 2, height // 2
for y in range(height):
    for x in range(width):
        dist = np.sqrt((x - center_x)**2 + (y - center_y)**2)
        if dist < 200:
            img[y, x] = 255

# Add some rectangles
img[100:200, 100:300] = 180
img[800:900, 700:900] = 200

# Add some diagonal lines
for i in range(0, min(width, height), 50):
    if i + 5 < width and i + 5 < height:
        img[i:i+5, i:i+5] = 255

# Add some noise
noise = np.random.randint(0, 50, (height, width), dtype=np.uint8)
img = np.clip(img.astype(np.int16) + noise, 0, 255).astype(np.uint8)

# Save as binary file
import os
output_dir = os.path.join(os.path.dirname(__file__), '..', 'test_data')
os.makedirs(output_dir, exist_ok=True)
output_path = os.path.join(output_dir, 'input.bin')

with open(output_path, 'wb') as f:
    f.write(img.tobytes())

print(f"Created test image: {output_path} ({width}x{height})")
