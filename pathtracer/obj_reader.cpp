// ===[A C K N O W L E D G E M E N T S]===
//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/

#include "obj_reader.hpp"

bool load_obj(char* path, std::vector<Vec3f> &out_vertices) {
    
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<Vec3f> temp_vertices;
    std::vector<Vec2f> temp_uvs;
    std::vector<Vec3f> temp_normals;

    FILE * file = fopen(path, "r");
    
    if( file == NULL ){
        printf("Impossible to open the file !\n");
        return false;
    }
    
    while(true) {
        char lineHeader[128];
        
        int res = fscanf(file,"%s",lineHeader);

        if (res == EOF) {
            break;
        }
        
        if ( strcmp( lineHeader, "v" ) == 0 ){
            Vec3f vertex;
            fscanf(file, "%f %f %f\n", &vertex[0], &vertex[1], &vertex[2] );
             printf("%f %f %f\n",vertex[0], vertex[1], vertex[2]);
            temp_vertices.push_back(vertex);
        }
        else if ( strcmp( lineHeader, "vn" ) == 0 ){
//            Vec3f normal;
//            fscanf(file, "%f %f %f\n", &normal[0], &normal[1], &normal[2]);
//            temp_normals.push_back(normal);
        }
        else if (strcmp(lineHeader,"f") == 0) {

            std::string vertex1, vertex2, vertex3;
            int vertexIndex[3], uvIndex[3], normalIndex[3];
            
            int matches = fscanf(file, "%d %d %d", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
            
            if (matches != 3) {
                printf("File can't be read by our simple parser\n");
                return false;
            }

//            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
//                                 &vertexIndex[0], &uvIndex[0], &normalIndex[0],
//                                 &vertexIndex[1], &uvIndex[1], &normalIndex[1],
//                                 &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
//
//            if (matches != 9) {
//                printf("File can't be read by our simple parser \
//                       : ( Try exporting with other options\n");
//                return false;
//            }
            
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            
//            uvIndices.push_back(uvIndex[0]);
//            uvIndices.push_back(uvIndex[1]);
//            uvIndices.push_back(uvIndex[2]);
//
//            normalIndices.push_back(normalIndex[0]);
//            normalIndices.push_back(normalIndex[1]);
//            normalIndices.push_back(normalIndex[2]);
        }
    }
    
    for(int i=0; i < vertexIndices.size(); i++ ){
        int vertexIndex = vertexIndices[i];
        Vec3f vertex = temp_vertices[ vertexIndex-1 ];
        out_vertices.push_back(vertex);
    }
    
    return true;
}