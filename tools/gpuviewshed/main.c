#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "gpu.h"

int main(int argc, char* argv[]){
    int status = 0;
    vs_heightmap_t heightmap = {};
    float resolution = 2;
    if (argc != 7){ printf("Expects 6 arguments\n"); return 1; }
    // name x y z rad tiles PNG
    //    0 1 2 3 4 5 6
    // 2km radius viewshed @ 20m AGL 
    // time ./viewshed -2.2331 51.8634 20 2 UK_2m_29_34.asc test_20.png

    float lon = atof(argv[1]);
    float lat = atof(argv[2]);
    uint32_t z = atoi(argv[3]);
    float radius = atof(argv[4]);
    FILE* viewshed_file = fopen(argv[6], "w");


    if (viewshed_file == NULL){ printf("Couldn't open viewshed file\n"); return 3; }

    // vs_heightmap_t heightmap = heightmap_from_file(heightmap_file);
    if( (status = heightmap_from_files(argv[5], &heightmap, &resolution)) != 0 ){
        fprintf(stderr, "Error opening heightmap file(s): %s \n", strerror(status));
        goto exit;
    }

    fprintf(stderr, "Heightmap Info:\n\trows: %u\n\tcols: %u\n\txll:  %.6f\n\tyll:  %.6f\n\tresolution:  %.0f\n", heightmap.rows, heightmap.cols, heightmap.xll, heightmap.yll, resolution);
    
    uint32_t x;
    uint32_t y;
    uint32_t radpx = (int)((radius*1000) / resolution);
    if( (heightmap_wgs84_to_xy(&heightmap, lon, lat, radius, &x, &y)) != 0 ){
        goto exit;
    }


    vs_viewshed_t viewshed = gpu_calculate_viewshed(heightmap, x, y, z, radpx);

    if( (status = viewshed_to_png(&viewshed, viewshed_file)) != 0 ){
        fprintf(stderr, "Error outputting viewshed\n");
        goto exit;
    }

exit:
    heightmap_destroy(&heightmap);

    return status;
}
