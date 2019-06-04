#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "gpu.h"

int main(int argc, char** argv){
    int status = 0;
    vs_heightmap_t heightmap = {};
    float resolution = -1;
    float north,east,south,west;
    int debug=1;
    // name x y z rad tiles PNG
    //    0 1 2 3 4 5 6
    // 2km radius viewshed @ 20m AGL 
    // time ./viewshed -2.2331 51.8634 20 2 UK_2m_29_34.asc test_20.png

    float lat, lon, radius=1;
    uint32_t TxH=1, RxH=1;
    FILE* viewshed_file;
    char *cvalue = NULL;
    char *pngfile = NULL;
    int index;
    int c,aflag=0,err=0;
    extern char *optarg;
    static char usage[] = "usage: %s -n latitude -e longitude [-a Tx height AGL] [-b Rx height AGL] [-r Radius (km)] -t ASC tiles (CSV) -p PNG filename [-v version]\n";
    static char example[] = "Eg: viewshed -n 52.123 -e -1.123 -a 8 -b 1 -r 2 -t DSM.asc -p viewshed.png\n";
    static char version[] = "\nCloudRF GPU viewshed tool v1.0\nOpen sourced for fun by Alex Farrant and Gareth Evans :p\n";
    char *tiles, *png;
    
    while ((c = getopt (argc, argv, "n:e:a:b:r:t:p:")) != -1){
      switch (c)
        {
        case 'n':
          lat = atof(optarg);
          break;
        case 'e':
          lon = atof(optarg);
          break;
        case 'a':
          TxH = atoi(optarg);
          break;
        case 'b':
          RxH = atoi(optarg);
          break;
        case 'r':
          radius = atof(optarg);
          break;
        case 't':
          tiles = optarg;
          break;
        case 'p':
          pngfile = optarg;
          break;
        case '?':
          err = 1;
          break;
        default:
          err = 1;
          break;
        }
    }

    // sanity check
    if(lat < -80 || lat > 80 || lon < -180 || lon > 180){
        fprintf(stderr,"Bad co-ordinates!\n");
        err=1;
    }
    if(TxH < 0 || TxH > 30e3 || RxH < 0 || RxH > 30e3){
        fprintf(stderr,"Bad Tx/RX height!\n");
        err=1;
    }
    if(radius < 0.1 || radius > 30){ // Update once curvature implemented!
        fprintf(stderr,"Bad radius!\n");
        err=1;
    }

    if(err){
        fprintf(stderr, usage, argv[0]);
        fprintf(stderr, example);
        fprintf(stderr, version);
        exit(1);
    }

    if(debug)
        fprintf(stderr,"Latitude: %.6f\nLongitude: %.6f\nTx height: %dm\nRx height: %dm\nRadius: %.1fkm\n",lat,lon,TxH,RxH,radius);

    viewshed_file = fopen(pngfile, "w");

    if (viewshed_file == NULL){ printf("Couldn't open viewshed file\n"); return 3; }

    // vs_heightmap_t heightmap = heightmap_from_file(heightmap_file);
    if( (status = heightmap_from_files(tiles, &heightmap, &resolution)) != 0 ){
        fprintf(stderr, "Error opening heightmap file(s): %s \n", strerror(status));
        goto exit;
    }

    if(debug)
        fprintf(stderr, "Heightmap Info:\n\trows: %u\n\tcols: %u\n\txll:  %.6f\n\tyll:  %.6f\n\tresolution:  %.0f\n", heightmap.rows, heightmap.cols, heightmap.xll, heightmap.yll, resolution);

    uint32_t x;
    uint32_t y;
    uint32_t radpx = (int)((radius*1000) / resolution);
    uint32_t ppd;
    if( (heightmap_wgs84_to_xy(&heightmap, lon, lat, &x, &y, &ppd)) != 0 ){
        goto exit;
    }

    // Sanity check cropping is possible within available heightmap
    if(x+radpx>heightmap.cols){
        //fprintf(stderr,"Cropping is %d pixels beyond X limit (%d)!\n",(x+radpx)-heightmap.cols,heightmap.cols);
        radpx-=(x+radpx)-heightmap.cols;
    }
    if(y+radpx>heightmap.rows){
        //fprintf(stderr,"Cropping is %d pixels beyond Y limit (%d)!\n",(y+radpx)-heightmap.rows,heightmap.rows);
        radpx-=(y+radpx)-heightmap.rows;
    }
    vs_viewshed_t viewshed = gpu_calculate_viewshed(heightmap, x, y, TxH, RxH, radpx);

    if( (status = viewshed_to_png(&viewshed, viewshed_file, x, y, radpx*2)) != 0 ){
        //fprintf(stderr, "Error outputting viewshed\n");
        goto exit;
    }
    int adj=3;
    north=lat+((radpx-adj)/(ppd*1.0));
    east=lon+(radpx/(ppd*1.0));
    south=lat-((radpx-adj)/(ppd*1.0));
    west=lon-(radpx/(ppd*1.0));
    fprintf(stdout,"|%.6f|%.6f|%.6f|%.6f|\n",north,east,south,west);

exit:
    heightmap_destroy(&heightmap);
    return status;
}
