#!/usr/bin/env python3

import csv
import os
import sys
import numpy as np

input_path = sys.argv[1]
target_row = int(sys.argv[2] if len(sys.argv) > 2 else "0")
label = sys.argv[3] if len(sys.argv) > 3 else None

with open(input_path, 'r') as input_file:
    reader = csv.reader(input_file)
    rows =  np.array([float(row[target_row]) for row in reader])
    
    average = np.average(rows)
    stddev = np.std(rows)
    max_ = np.max(rows)
    min_ = np.min(rows)

    if label is None:
        print('|{0:0.0f}|{1:0.5f}|{2:0.0f}|{3:0.0f}|'.format(average, stddev, max_, min_))
    else:
        print('|{0}|{1:0.0f}|{2:0.5f}|{3:0.0f}|{4:0.0f}|'.format(label, average, stddev, max_, min_))
