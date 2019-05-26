#!/bin/sh

# Arle court roundabout, 30m AGL
./viewshed -2.2002 51.8788 30 ../testdata/Gloucester_2m.asc test1.asc && gdaldem hillshade -of JPEG test1.asc test1.jpg
