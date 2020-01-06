#!/usr/bin/env python

import numpy as np
import collections

FFT_SIZE   = 256
BURST_SIZE = 16

coord       = np.loadtxt("coord.txt")
cell        = coord[:,0]*FFT_SIZE + coord[:,1]

keys_values = sorted(collections.Counter(cell).items())
keys        = np.array(keys_values)[:,0]
values      = np.array(keys_values)[:,1]

start = sorted([list(cell).index(x) for x in list(keys)])
count = values

start_count = np.zeros((FFT_SIZE*FFT_SIZE, 2))
start_count[keys.astype(int), 0] = start
start_count[keys.astype(int), 1] = count
print sorted(count)

np.savetxt("start_count.txt", start_count, fmt='%d')
np.savetxt("count.txt", count, fmt='%d')
np.savetxt("start.txt", start, fmt='%d')

count_burst = start_count[:,1].reshape(FFT_SIZE*FFT_SIZE/BURST_SIZE, BURST_SIZE)
#print sorted(np.sum(count_burst, axis=1))
