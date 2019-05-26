#!/bin/sh

# GPU?
nvidia-debugdump -l

# Unpack LIDAR
tar xvzf ../testdata/2m_lidar.tgz

# Single tile, Arle court roundabout, 30m AGL
# 5k x 5k points = 25MP
time ./viewshed -2.2002 51.8788 30 UK_2m_29_34.asc test1.asc && gdaldem hillshade -of JPEG test1.asc test1.jpg

# Four tiles, Crypt school, 30m AGL
# 10k x 10k points = 100MP
# GeForce GTX 750 Ti = 28s
time ./viewshed -2.2516 51.8394 30 UK_2m_29_33.asc,UK_2m_29_34.asc,UK_2m_30_33.asc,UK_2m_30_34.asc test2.asc && gdaldem hillshade -of JPEG test2.asc test2.jpg

# Four tiles, Crypt school, 300m AGL
# 10k x 10k points = 100MP
# GeForce GTX 750 Ti = 28s
time ./viewshed -2.2516 51.8394 300 UK_2m_29_33.asc,UK_2m_29_34.asc,UK_2m_30_33.asc,UK_2m_30_34.asc test3.asc && gdaldem hillshade -of JPEG test3.asc test3.jpg
