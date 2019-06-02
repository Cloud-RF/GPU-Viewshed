#include <math.h>

#include "cpu.h"
#include "common.h"

double curvature_drop(double distance){
    /*
     *     |<--- d ---->|
     *   _ +------------+   _
     *   ^ |~ ~ - ,     |   ^
     *   | |        ' , |   o_2
     *   | |           .,   v
     *   R |       . `  |,  ^
     *   | |     .'     | , o_1
     *   | |   .`       | ,
     *   v | .'         | , v
     *   - +------------+   -
     *     |<--- d ---->|
     *
     *
     *      R = radius of the earth = 6,371,000m
     *      d = distance from centre point
     *
     *      R^2 = d^2 + o_1^2  -- rearrage for o_1
     *      o_2 = R - o_1
     *
     *      o_2 = R - o_1 = R - sqrt(R^2 - d^2)
     */

    double R = 6371000;
    return -(R - sqrt(R*R - distance*distance));
}

void curve_map(vs_heightmap_t map){
    uint32_t mid_x = map.cols / 2;
    uint32_t mid_y = map.rows / 2;

    for (uint32_t y = 0; y < map.rows; y++)
    for (uint32_t x = 0; x < map.cols; x++){
        uint32_t dx = (x > mid_x ? x-mid_x : mid_x-x) * map.cellsize;
        uint32_t dy = (y > mid_y ? y-mid_y : mid_y-y) * map.cellsize;

        map.heightmap[y*map.cols + x] += curvature_drop(sqrt(dx*dx + dy*dy));
    }
    return;
}

vs_viewshed_t calculate_viewshed(vs_heightmap_t heightmap, uint32_t emitter_x, uint32_t emitter_y, uint32_t emitter_z, uint32_t radius){
    vs_viewshed_t viewshed = viewshed_from_heightmap(heightmap);

    for (uint32_t ty = 0; ty < heightmap.rows; ty++)
    for (uint32_t tx = 0; tx < heightmap.cols; tx++){
        uint32_t x = tx;
        uint32_t y = ty;
        int dx = (x > emitter_x ? x - emitter_x : emitter_x - x), sx = x < emitter_x ? 1 : -1;
        int dy = (y > emitter_y ? y - emitter_y : emitter_y - y), sy = y < emitter_y ? 1 : -1;
        int dz = emitter_z - heightmap.heightmap[y*heightmap.cols+x];
        int err = (dx>dy ? dx : -dy)/2, e2;

        float distance = sqrt(dx*dx + dy*dy);

        bool visible = true;
        if (distance == 0 && dz > 0){ viewshed.viewshed[y*viewshed.cols+x] = visible; continue; }
        for (;;){
            if(distance > radius){ break; }       
            float fraction = sqrt(dx*dx + dy*dy) / distance;
            int height_offset = fraction * dz;
            if (heightmap.heightmap[y*heightmap.cols+x] > emitter_z - height_offset){ visible = false; }
            if (x == emitter_x && y == emitter_y){ break; }
            e2 = err;
            if (e2 > -dx) { err -= dy; x += sx; }
            if (e2 <  dy) { err += dx; y += sy; }
        }
        viewshed.viewshed[ty*viewshed.cols+tx] = visible;
    }

    return viewshed;
}
