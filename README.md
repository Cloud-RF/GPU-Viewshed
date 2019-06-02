GPU accelerated viewshed calculator
===================================

An open source CLI tool for fast viewshed generation using [OpenCL](https://www.khronos.org/opencl/).
Based upon a Software Engineering [Masters project](https://github.com/AbstractBeliefs/hons) by [Gareth Pulham MEng](https://github.com/AbstractBeliefs).
Modified for server integration by [Alex Farrant](https://github.com/Cloud-RF) and [Gareth Evans](https://github.com/kryc).

## Requirements
* GPU graphics card
* OpenCL
* clang
* libPNG

### Preparation (Ubuntu)
`apt install nvidia-driver-390 clang ocl-icd-opencl-dev`

## Build

`make all`

## Test
`./test.sh`

## Arguments
-n latitude (WGS84 Decimal degrees)

-e longitude  (WGS84 Decimal degrees)

-a Tx height AGL (m)

-b Rx height AGL (m) 

-r Radius (km)

-t ASCII grid tiles (Comma separated) 

-p PNG filename


## Example
./viewshed -n 52.123 -e -1.123 -a 8 -b 1 -r 2 -t DSM.asc -p viewshed.png

