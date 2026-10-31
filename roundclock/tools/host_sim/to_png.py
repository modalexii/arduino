#!/usr/bin/env python3
"""Convert a 466x466 RGB565 frame from sim to PNG: python3 to_png.py in.raw out.png"""
import sys
import numpy as np
from PIL import Image
a = np.fromfile(sys.argv[1], dtype="<u2").reshape(466, 466).astype(np.uint32)
rgb = np.dstack([((a >> 11) & 31) * 255 // 31, ((a >> 5) & 63) * 255 // 63, (a & 31) * 255 // 31])
Image.fromarray(rgb.astype(np.uint8), "RGB").save(sys.argv[2])
