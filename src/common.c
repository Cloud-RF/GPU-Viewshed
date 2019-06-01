#include "common.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <png.h>
#include <assert.h>
#include <dlfcn.h>

typedef struct _coord_t{
    float x;
    float y;
} coord_t, *pcoord_t;

typedef struct __attribute__((packed)) _rgb_t{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t, *prgbt;

 #define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

double haversine_formula(double th1, double ph1, double th2, double ph2)
{
    #define TO_RAD (3.1415926536 / 180)
    int R = 6371;
    double dx, dy, dz;
    ph1 -= ph2;
    ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;
    dz = sin(th1) - sin(th2);
    dx = cos(ph1) * cos(th1) - cos(th2);
    dy = sin(ph1) * cos(th1);
    return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R;
}

static inline int
heightmap_get_resolution(vs_heightmap_t *heightmap, float *precise, float *rounded){
    int status = 0;
    double current_res_km;
    double precise_resolution;
    double rounded_resolution;

    if( precise != NULL )
        *precise = 0.0;
    if( rounded != NULL )
        *rounded = 0.0;

    current_res_km = haversine_formula(heightmap->yll + (heightmap->cellsize * heightmap->rows),     
                                       heightmap->xll,
                                       heightmap->yll + (heightmap->cellsize * heightmap->rows),
                                       heightmap->xll + (heightmap->cellsize * heightmap->cols));
    
    precise_resolution = (current_res_km/max(heightmap->cols, heightmap->rows)*1000);
    // Round to nearest 0.5
    rounded_resolution = precise_resolution < 0.5f ? 0.5f : ceil((precise_resolution * 2)+0.5) / 2;
    
    if( precise != NULL )
        *precise = precise_resolution;
    if( rounded != NULL )
        *rounded = rounded_resolution;

exit:
    return status;
}

static inline int
heightmap_get_ppd(vs_heightmap_t *heightmap, size_t *ppdx, size_t *ppdy){
    int status = 0;

    double width_deg = heightmap->cellsize * heightmap->cols;
    double height_deg = heightmap->cellsize * heightmap->rows;

    *ppdx = heightmap->cols / width_deg;
    *ppdy = heightmap->rows / height_deg;

exit:
    return status;
}

int
heightmap_wgs84_to_xy(vs_heightmap_t *heightmap, float lon, float lat, uint32_t *x, uint32_t *y, uint32_t *ppd){
    int status = 0;
    *x = 0;
    *y = 0;

    float min_lon = heightmap->xll;
    float max_lon = heightmap->xll + (heightmap->cellsize * heightmap->cols);
    float min_lat = heightmap->yll;
    float max_lat = heightmap->yll + (heightmap->cellsize * heightmap->rows);
    if( lon < min_lon || lon > max_lon ){
        fprintf(stderr, "Error: Longitude not in range: %.6f [%.6f:%.6f]\n", lon, min_lon, max_lon);
        status = EINVAL;
        goto exit;
    }else if( lat < min_lat || lat > max_lat ){
        fprintf(stderr, "Error: Latitude not in range: %.6f [%.6f:%.6f]\n", lat, min_lat, max_lat);
        status = EINVAL;
        goto exit;
    }
    size_t ppdx;
    size_t ppdy;

    if( (status = heightmap_get_ppd(heightmap, &ppdx, &ppdy)) != 0 ){
        goto exit;
    }
    *ppd=ppdx;
    *x = (uint32_t)((lon - min_lon) * ppdx);
    *y = (uint32_t)(heightmap->rows - (lat - min_lat) * ppdy);
exit:
    return status;
}

vs_heightmap_t heightmap_from_array(uint32_t rows, uint32_t cols, int *input){
    vs_heightmap_t heightmap;

    heightmap.rows = rows;
    heightmap.cols = cols;
    heightmap.cellsize = 1;

    heightmap.corner = true;
    heightmap.xll = 0;
    heightmap.yll = 0;

    heightmap.nodata = -9999;
    heightmap.heightmap = calloc(rows * cols, sizeof(float));

    memcpy(heightmap.heightmap, input, rows*cols*sizeof(float));

    return heightmap;
}

// ASCII Grid https://en.wikipedia.org/wiki/Esri_grid
static vs_heightmap_t
heightmap_from_file_asc(FILE* inputfile){
    vs_heightmap_t map;
    map.rows = 0;
    map.cols = 0;
    map.cellsize = 1;

    map.corner = true;
    map.xll = 0;
    map.yll = 0;
    map.heightmap = NULL;

    map.nodata = -9999;

    // Get the file length
    fseek(inputfile, 0, SEEK_END);
    size_t fsize = ftell(inputfile);
    rewind(inputfile);

    // Allocate buffers
    char* buffer = calloc(fsize, sizeof(char)); // TODO: needs errorchecked
    size_t result = fread(buffer, 1, fsize, inputfile); // TODO: needs errorchecked for != fsize
    buffer[fsize+sizeof(char)-1] = '\0';

    // Parse file
    bool r_parse = false;
    bool c_parse = false;
    size_t cellcount = 0;

    char* fragment = strtok(buffer, " \n");
    while (fragment != NULL){
        if (!strcasecmp(fragment, "NROWS")){
            fragment = strtok(NULL, " \n"); map.rows = atoi(fragment);
            if ((r_parse = true) && c_parse){ map.heightmap = calloc(map.rows * map.cols, sizeof(float)); }
        }
        else if (!strcasecmp(fragment, "NCOLS")){
            fragment = strtok(NULL, " \n"); map.cols = atoi(fragment);
            if ((c_parse = true) && r_parse){ map.heightmap = calloc(map.rows * map.cols, sizeof(float)); }
        }
        else if (!strcasecmp(fragment, "XLLCORNER") || !strcasecmp(fragment, "XLLCENTER")){
            if (!strcasecmp(fragment, "XLLCORNER")){ map.corner = true; } else { map.corner = false; }
            fragment = strtok(NULL, " \n"); map.xll = atof(fragment);
        }
        else if (!strcasecmp(fragment, "YLLCORNER") || !strcasecmp(fragment, "YLLCENTER")){
            if (!strcasecmp(fragment, "YLLCORNER")){ map.corner = true; } else { map.corner = false; }
            fragment = strtok(NULL, " \n"); map.yll = atof(fragment);
        }
        else if (!strcasecmp(fragment, "CELLSIZE")){ fragment = strtok(NULL, " \n"); map.cellsize = atof(fragment); }
        else if (!strcasecmp(fragment, "NODATA_VALUE")){ fragment = strtok(NULL, " \n"); map.nodata = atof(fragment); }
        else {
            if (map.heightmap != NULL && cellcount < map.cols*map.rows){
                map.heightmap[cellcount++] = atof(fragment);
            }
        }

        fragment = strtok(NULL, " \n");
    }

    return map;
}

vs_heightmap_t
heightmap_from_file(FILE* inputfile){
    char *lib_name = getenv("HEIGHTMAP_LOADER");
    if( lib_name != NULL &&
        access( lib_name, F_OK ) != -1 ) {
        void *handle;

        if( (handle = dlopen(lib_name, RTLD_LAZY)) == NULL ){
            fprintf(stderr, "Error opening heightmap loader: %s\n", dlerror());
            exit(errno);
        }

        vs_heightmap_t (*fn)(FILE*);
        if( (fn = dlsym(handle, "heightmap_from_file")) == NULL ){
            fprintf(stderr, "Error loading heightmap loader: %s\n", dlerror());
            exit(errno);
        }

        vs_heightmap_t result = fn(inputfile);
        dlclose(handle);
        return result;
    }

    return heightmap_from_file_asc(inputfile);

}

void
heightmap_destroy(vs_heightmap_t * const heightmap){
    if( heightmap->heightmap != NULL ){
        free(heightmap->heightmap);
        heightmap->heightmap = NULL;
    }
}

static int
parse_input_files(const char * const inputfiles,
                  size_t * const file_count,
                  vs_heightmap_t ** tile_array){
    int status = 0;
    char *tmp_inputfiles;
    char **filenames;
    size_t count = 0;
    vs_heightmap_t *tiles = NULL;

    *file_count = 0;
    *tile_array = NULL;

    /* Copy the list as strtok is a destructive operation */
    if( (tmp_inputfiles = calloc(strlen(inputfiles)+1, sizeof(char))) == NULL ){
        status = ENOMEM;
        goto exit;
    }
    strcpy(tmp_inputfiles, inputfiles);

    /* Parse out the filenames from the list provided */
    if( (filenames = (char**)malloc(sizeof(char*))) == NULL ){
        status = ENOMEM;
        goto exit;
    }
    filenames[count] = strtok(tmp_inputfiles, ",");
    while(filenames[count] != NULL){
        count++;
        if( (filenames = (char**)realloc(filenames, sizeof(char*)*(count+1))) == NULL ){
            status = ENOMEM;
            goto exit;
        }
        filenames[count] = strtok(NULL, ",");
    }

    /* Read each file into a vs_heightmap_t */
    tiles = (vs_heightmap_t*)calloc(count, sizeof(vs_heightmap_t));
    for(size_t i=0; i<count; i++){
        fprintf(stderr, "Loading file: %s\n", filenames[i]);
        FILE *fp = fopen(filenames[i], "r");
        if( fp == NULL ){
            status = ENOENT;
            goto exit;
        }
        tiles[i] = heightmap_from_file(fp);
        fclose(fp);
    }

    *file_count = count;
    *tile_array = tiles;

exit:
    if( tmp_inputfiles != NULL ){
        free(tmp_inputfiles);
    }
    if( filenames != NULL ){
        free(filenames);
    }
    if( status != 0 && tiles != NULL ){
        for(size_t i=0; i<count; i++){
            heightmap_destroy(&tiles[i]);
        }
        free(tiles);
    }

    return status;
}

int
heightmap_from_files(const char * const inputfiles,
                        vs_heightmap_t * const map, float *tileResolution){
    int status = 0;
    size_t file_count;
    vs_heightmap_t *tiles;
    float resolution;
    if( (status = parse_input_files(inputfiles, &file_count, &tiles)) != 0 ){
        goto exit;
    }

    /* If there is only one file, no need to join them together */
    if( file_count == 1 ){
        *map = tiles[0];
        status = heightmap_get_resolution(&tiles[0], NULL, &resolution); 
        goto exit;
    }

    /* Create one large tile */
    coord_t lower_left;
    coord_t upper_right;
    uint16_t cellsize;
    uint32_t rows;
    uint32_t cols;
    float max_resolution;
    float min_resolution;
    int nodata;
    for(size_t i=0; i<file_count; i++){
        coord_t next_ll;
        coord_t next_ur;

        next_ll.x = tiles[i].xll;
        next_ll.y = tiles[i].yll;
        next_ur.x = tiles[i].xll + (tiles[i].cellsize * tiles[i].cols);
        next_ur.y = tiles[i].yll + (tiles[i].cellsize * tiles[i].rows);

        if( (status = heightmap_get_resolution(&tiles[i], NULL, &resolution)) != 0 ){
            goto exit;
        }

        if( i == 0 ){
            lower_left = next_ll;
            upper_right = next_ur;
            min_resolution = resolution;
            max_resolution = resolution;
            nodata = tiles[i].nodata;
        }else{
            lower_left.x = fmin(next_ll.x, lower_left.x);
            lower_left.y = fmin(next_ll.y, lower_left.y);
            upper_right.x = fmax(next_ur.x, upper_right.x);
            upper_right.y = fmax(next_ur.y, upper_right.y);
            min_resolution = fmin(resolution, min_resolution);
            max_resolution = fmax(resolution, max_resolution);
        }
    }

    /* TODO: At present, we don't support tile resampling */
    if( max_resolution != min_resolution ){
        status = ENOSYS;
        goto exit;
    }


    /* Now that we have the size of the giant tile, we need to stitch them together */
    float width = upper_right.x - lower_left.x;
    float height = upper_right.y - lower_left.y;
    size_t new_height = 0;
    size_t new_width = 0;
    for( size_t i=0; i<file_count; i++ ){
        float north_offset = upper_right.y - (tiles[i].yll + (tiles[i].cellsize * tiles[i].rows));
        float west_offset = fmax(tiles[i].xll - lower_left.x, 0); // Catch minor floating point errors
        size_t ppdx;
        size_t ppdy;
        if( (status = heightmap_get_ppd(&tiles[i], &ppdx, &ppdy)) != 0 ){
            goto exit;
        }
        
        size_t north_pixel_offset = (size_t)(north_offset * ppdy);
        size_t west_pixel_offset = (size_t)(round((west_offset * ppdx)/10)*10); // eg. 4999 = 5000

        new_width = max(west_pixel_offset + tiles[i].cols, new_width);
        new_height = max(north_pixel_offset + tiles[i].rows, new_height);
    }

    size_t new_tile_alloc = new_width * new_height;
    float *new_tile = NULL;
    if( (new_tile = calloc( new_tile_alloc, sizeof(float) )) == NULL ){
        status = ENOMEM;
        goto exit;
    }

    for( size_t i=0; i<file_count; i++ ){
        float north_offset = upper_right.y - (tiles[i].yll + (tiles[i].cellsize * tiles[i].rows));
        float west_offset = fmax(tiles[i].xll - lower_left.x, 0); // Catch minor floating point errors
        size_t ppdx;
        size_t ppdy;
        if( (status = heightmap_get_ppd(&tiles[i], &ppdx, &ppdy)) != 0 ){
            goto exit;
        }

        size_t north_pixel_offset = (size_t)(north_offset * ppdy);
        size_t west_pixel_offset = (size_t)(west_offset * ppdx);

        /* Copy it row-by-row from the tile */
        for (size_t h = 0; h < tiles[i].rows; h++) {
            float *dest_addr = &new_tile[ (north_pixel_offset+h)*new_width + west_pixel_offset];
            float *src_addr = &tiles[i].heightmap[h*tiles[i].cols];
            // Check if we might overflow
            if ( dest_addr + tiles[i].cols > new_tile + new_tile_alloc || dest_addr < new_tile ){
                fprintf(stderr, "[!] Overflow detected: %zu\n", i);
                status = EFAULT;
                goto exit;
            }
            memcpy( dest_addr, src_addr, tiles[i].cols * sizeof(float) );
        }
    }
    
    map->rows = new_height;
    map->cols = new_width;
    map->cellsize = (upper_right.x - lower_left.x) / new_width;
    map->corner = tiles[0].corner; // Copy from first tile
    map->xll = lower_left.x;
    map->yll = lower_left.y;
    map->nodata = nodata;
    map->heightmap = new_tile;

exit:
    if( file_count != 1 && tiles != NULL ){
        for(size_t i=0; i<file_count; i++){
            heightmap_destroy(&tiles[i]);
        }
        free(tiles);
    }
    if( status != 0 && new_tile != NULL ){
        free(new_tile);
    }

    *tileResolution=resolution;

    fprintf(stderr, "Using resolution: %.1f\n", resolution);

    return status;
}

void heightmap_to_file(vs_heightmap_t heightmap, FILE* outputfile){
    fprintf(outputfile, "NCOLS %d\n", heightmap.cols);
    fprintf(outputfile, "NROWS %d\n", heightmap.rows);
    fprintf(outputfile, "XLL%s %.6f\n", (heightmap.corner ? "CORNER" : "CENTER"), heightmap.xll);
    fprintf(outputfile, "YLL%s %.6f\n", (heightmap.corner ? "CORNER" : "CENTER"), heightmap.yll);
    fprintf(outputfile, "CELLSIZE %.6f\n", heightmap.cellsize);

    for (size_t row = 0; row < heightmap.rows; row++){
        for (size_t col = 0; col < heightmap.cols; col++){
            fprintf(outputfile, "%f ", heightmap.heightmap[row*heightmap.cols+col]);
        }
        fseek(outputfile, -1, SEEK_CUR);
        fprintf(outputfile, "\n");
    }
    fflush(outputfile);
    return;
}

int
viewshed_to_png(vs_viewshed_t *viewshed, FILE *outputfile, int x, int y, int crop){
    int status;
    prgbt image_buffer;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    rgb_t black = {0,0,0};

    assert(sizeof(rgb_t) == 3);

    /* Create a white image buffer */
    size_t image_size = viewshed->rows * viewshed->cols * sizeof(rgb_t);
    if( (image_buffer = malloc(image_size)) == NULL ){
        status = ENOMEM;
        goto exit;
    }
    memset((void*)image_buffer, 0xff, image_size);

    /* Loop through and set the remaining pixels */
    for(size_t x=0; x<viewshed->cols; x++)
    for(size_t y=0; y<viewshed->rows; y++){
        if( viewshed->viewshed[(x * viewshed->cols) + y] )
            image_buffer[(x * viewshed->cols) + y] = black;
    }

    /* Initialize the row pointers */
    if( (row_pointers = (png_bytep*) calloc(viewshed->rows, sizeof(png_bytep))) == NULL ){
        status = ENOMEM;
        goto exit;
    }

    y-=(crop/2); // top of circle
    x-=(crop/2); // left of circle
    for(size_t i = 0; i<crop; i++){
        row_pointers[i] = (png_bytep) &image_buffer[((i+y) * viewshed->rows)+x];
    }

    /* Do PNG image processing */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png_ptr == NULL){
        status = ENOMEM;
        goto exit;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL){
        status = ENOMEM;
        goto cleanup;
    }

    if (setjmp(png_jmpbuf(png_ptr))){
        status = EINVAL;
        goto cleanup;
    }

    png_init_io(png_ptr, outputfile);

#ifdef COMPRESSION
    png_set_compression_level(png_ptr, COMPRESSION);
#endif

    png_set_IHDR(png_ptr, info_ptr, crop, crop, 8 /*bit_depth*/,
                    PNG_COLOR_TYPE_RGB,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
    status = 0;

cleanup:

    png_destroy_write_struct(&png_ptr, &info_ptr);

exit:

    if( image_buffer != NULL ){
        free(image_buffer);
    }

    if( row_pointers != NULL ){
        free(row_pointers);
    }

    return status;
}

vs_viewshed_t viewshed_from_array(uint32_t rows, uint32_t cols, bool *input){
    vs_viewshed_t viewshed;

    viewshed.rows = rows;
    viewshed.cols = cols;
    viewshed.cellsize = 1;

    viewshed.corner = true;
    viewshed.xll = 0;
    viewshed.yll = 0;

    viewshed.viewshed = calloc(rows * cols, sizeof(bool));

    memcpy(viewshed.viewshed, input, rows*cols*sizeof(bool));

    return viewshed;
}

void viewshed_to_file(vs_viewshed_t viewshed, FILE* outputfile){
    fprintf(outputfile, "NCOLS %d\n", viewshed.cols);
    fprintf(outputfile, "NROWS %d\n", viewshed.rows);
    fprintf(outputfile, "XLL%s %.6f\n", (viewshed.corner ? "CORNER" : "CENTER"), viewshed.xll);
    fprintf(outputfile, "YLL%s %.6f\n", (viewshed.corner ? "CORNER" : "CENTER"), viewshed.yll);
    fprintf(outputfile, "CELLSIZE %.6f\n", viewshed.cellsize);

    for (size_t row = 0; row < viewshed.rows; row++){
        for (size_t col = 0; col < viewshed.cols; col++){
            fprintf(outputfile, "%d ", viewshed.viewshed[row*viewshed.cols+col]);
        }
        fseek(outputfile, -1, SEEK_CUR);
        fprintf(outputfile, "\n");
    }
    fflush(outputfile);
    return;
}

vs_viewshed_t viewshed_from_heightmap(vs_heightmap_t heightmap){
    vs_viewshed_t viewshed;

    viewshed.rows = heightmap.rows;
    viewshed.cols = heightmap.cols;
    viewshed.cellsize = heightmap.cellsize;

    viewshed.corner = heightmap.corner;
    viewshed.xll = heightmap.xll;
    viewshed.yll = heightmap.yll;

    viewshed.viewshed = calloc(viewshed.rows * viewshed.cols, sizeof(bool));

    return viewshed;
}

