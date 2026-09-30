#!/usr/bin/env python3

import sys
import numpy as np
import cv2

row = []
reading = 0

with open(sys.argv[1]) as file:
    for line in file:
        if reading:
            if line == 'END_DATA\n':
                break
            color = line.split()[1:4]
            row.append([
                float(color[2]) * (2**16 - 1),
                float(color[1]) * (2**16 - 1),
                float(color[0]) * (2**16 - 1),
            ])  # bgr
        else:
            if line == 'BEGIN_DATA\n':
                reading = 1
            elif line.startswith('NUMBER_OF_SETS'):
                sets = int(line.split()[1])

if len(row) == sets:
    image = np.array([row], dtype=np.uint16)
    print(image)
    cv2.imwrite(sys.argv[2], image)
