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

    if (argc != 6){ printf("Expects 5 arguments\n"); return 1; }
    // name x y z if of
    //    0 1 2 3  4  5
    FILE* viewshed_file = fopen(argv[5], "w");

    float lon = atof(argv[1]);
    float lat = atof(argv[2]);
    uint32_t z = atoi(argv[3]);

    if (viewshed_file == NULL){ printf("Couldn't open viewshed file\n"); return 3; }

    // vs_heightmap_t heightmap = heightmap_from_file(heightmap_file);
    if( (status = heightmap_from_files(argv[4], &heightmap)) != 0 ){
        fprintf(stderr, "Error opening heightmap file(s): %s \n", strerror(status));
        goto exit;
    }

    fprintf(stderr, "Heightmap Info:\n\trows: %u\n\tcols: %u\n\txll:  %.6f\n\tyll:  %.6f\n", heightmap.rows, heightmap.cols, heightmap.xll, heightmap.yll);
    
    uint32_t x;
    uint32_t y;
    if( (heightmap_wgs84_to_xy(&heightmap, lon, lat, &x, &y)) != 0 ){
        goto exit;
    }

    vs_viewshed_t viewshed = gpu_calculate_viewshed(heightmap, x, y, z);

    viewshed_to_file(viewshed, viewshed_file);

exit:
    heightmap_destroy(&heightmap);

    return status;
}
