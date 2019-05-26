#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "gpu.h"

int main(int argc, char* argv[]){
    int status = 0;
    vs_heightmap_t heightmap;
    
    if (argc != 6){ printf("Expects 5 arguments\n"); return 1; }
    // name x y z if of
    //    0 1 2 3  4  5
    FILE* viewshed_file = fopen(argv[5], "w");

    uint32_t x = atoi(argv[1]);
    uint32_t y = atoi(argv[2]);
    uint32_t z = atoi(argv[3]);

    if (viewshed_file == NULL){ printf("Couldn't open viewshed file\n"); return 3; }

    // vs_heightmap_t heightmap = heightmap_from_file(heightmap_file);
    if( (status = heightmap_from_files(argv[4], &heightmap)) != 0 ){
        fprintf(stderr, "Error opening heightmap file(s): %s \n", strerror(status));
        return status;
    }

    fprintf(stderr, "Heightmap Info:\n\trows: %u\n\tcols: %u\n", heightmap.rows, heightmap.cols);

    vs_viewshed_t viewshed = gpu_calculate_viewshed(heightmap, x, y, z);

    viewshed_to_file(viewshed, viewshed_file);

    return status;
}
