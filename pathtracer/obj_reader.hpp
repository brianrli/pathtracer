#ifndef __pathtracer__obj_reader__
#define __pathtracer__obj_reader__

#include "common.hpp"
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

bool load_obj(char* path, std::vector<Vec3f> &out_vertices);
//, std::vector <Vec3f> out_vertices);

#endif /* defined(__pathtracer__obj_reader__) */
