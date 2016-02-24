#ifndef __pathtracer__obj_reader__
#define __pathtracer__obj_reader__

#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "common.hpp"

bool load_obj(char* path, std::vector<Vec3f> &out_vertices);
//, std::vector <Vec3f> out_vertices);

#endif /* defined(__pathtracer__obj_reader__) */
