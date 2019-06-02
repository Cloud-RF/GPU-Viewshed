#!/bin/sh

# GPU?
nvidia-debugdump -l

# Unpack LIDAR
tar xvzf testdata/2m_lidar.tgz

# Single tile, Arle court roundabout, 8m AGL, 2km radius
# 5k x 5k points = 25MP
# GeForce GTX 750 Ti = 3s
time ./tools/gpuviewshed/viewshed -n 51.8788 -e -2.2002 -a 8 -b 1 -r 2 -t UK_2m_29_34.asc -p 2km.png

# Four tiles, Crypt school, 12m AGL, 5km radius
# 10k x 10k points = 100MP
# GeForce GTX 750 Ti = 14s
time ./tools/gpuviewshed/viewshed -n 51.8394 -e -2.2516 -a 8 -b 1 -r 5 -t UK_2m_29_33.asc,UK_2m_29_34.asc,UK_2m_30_33.asc,UK_2m_30_34.asc -p 5km.png


