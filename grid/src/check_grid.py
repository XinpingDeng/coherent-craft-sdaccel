#!/usr/bin/env python

import numpy as np

coord   = np.loadtxt("start_count.txt")
err_loc = np.loadtxt("error.txt")

error_coord = coord[err_loc.astype(int)]

print (error_coord[:,0] + 1.)/16.
print len(err_loc)*16
