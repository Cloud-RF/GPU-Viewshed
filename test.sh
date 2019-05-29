#!/bin/sh

# GPU?
nvidia-debugdump -l

# Unpack LIDAR
tar xvzf testdata/2m_lidar.tgz

# Single tile, Arle court roundabout, 8m AGL, 2km radius
# 5k x 5k points = 25MP
time ./tools/gpuviewshed/viewshed -2.2002 51.8788 8 2 UK_2m_29_34.asc 2km.png

# Four tiles, Crypt school, 12m AGL, 5km radius
# 10k x 10k points = 100MP
# GeForce GTX 750 Ti = 28s
time ./tools/gpuviewshed/viewshed -2.2516 51.8394 12 5 UK_2m_29_33.asc,UK_2m_29_34.asc,UK_2m_30_33.asc,UK_2m_30_34.asc 5km.png


