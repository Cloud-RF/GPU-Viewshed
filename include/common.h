#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t rows;      // Size and shape
    uint32_t cols;      // of grid
    float cellsize;

    bool corner;        // Corner or center origin
    float xll;        // x-lower-left corner/center
    float yll;        // y-lower-left corner/center

    int32_t nodata;     // null value. Default -9999

    float* heightmap; // Data goes here
} vs_heightmap_t;

typedef struct {
    uint32_t rows;      // Size and shape
    uint32_t cols;      // of grid
    float cellsize;

    bool corner;        // Corner or center origin
    float xll;        // x-lower-left corner/center
    float yll;        // y-lower-left corner/center

    bool* viewshed;     // Data goes here
} vs_viewshed_t;

vs_heightmap_t heightmap_from_array(uint32_t rows, uint32_t cols, float *input);
vs_heightmap_t heightmap_from_file(FILE* inputfile);
int heightmap_from_files(const char * const inputfiles, vs_heightmap_t * const map);
void heightmap_to_file(vs_heightmap_t heightmap, FILE* outputfile);
void heightmap_destroy(vs_heightmap_t * const heightmap);
int heightmap_wgs84_to_xy(vs_heightmap_t *heightmap, float lon, float lat, uint32_t *x, uint32_t *y);

vs_viewshed_t viewshed_from_array(uint32_t rows, uint32_t cols, bool *input);
void viewshed_to_file(vs_viewshed_t viewshed, FILE* outputfile);
vs_viewshed_t viewshed_from_heightmap(vs_heightmap_t heightmap);

#ifdef __cplusplus
}
#endif
