#ifndef __pathtracer__png_writer__
#define __pathtracer__png_writer__

#include <stdio.h>
#include "common.hpp"
#include "/usr/local/include/png.h"

int save_png_to_file (Bitmap *bitmap, const char *path);
float clip(float n, float lower, float upper);

#endif



