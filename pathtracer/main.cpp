// ========================================
// ===[A C K N O W L E D G E M E N T S]====
//The GUI code is taken from Peter and Karl's GPU Pathtracer
//https://github.com/peterkutz/GPUPathTracer

//The OBJ Reader code is modified from:
//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/

//The Bounding Volume Hierarchy Code is modified
//from pbrt-v2
//https://github.com/mmp/pbrt-v2

// =========================================

// =========================================
// ===[B R I A N'S  P A T H  T R A C E R]===
// cpsc 490 at Yale
// advised by Professor Rushmeier
// =========================================

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "pathtracer.hpp"
#include "opencl_manager.hpp"
#include "png_writer.hpp"
#include "obj_reader.hpp"

#include <ctime>
#include <cstdio>

#define PATH_TRACER_BITMAP 13
#pragma OPENCL EXTENSION cl_khr_fp16 : enable


// constants
const bool DEBUG_MODE = false;

Pathtracer *pathtracer = NULL;
int window_height = 100;
int window_width = 100;

int size = window_height * window_width;
int iterations = 0;

// forward declarations
void initialize(int argc, char**argv);
bool initGL();

void errorm(std::string error_message) {
    std::cout << error_message << std::endl;
    exit(0);
}

// ===[ M A I N ]===
int main(int argc, char **argv) {
    
    if(DEBUG_MODE) {
        window_height = 1;
        window_width = 1;
    }
    
//    BoundingBox bbox;
//    Vec3f v1 = Vec3f(0.0179529,1.19773,0.449576);
//    Vec3f v2 = Vec3f(0.0572688, 0.0529851, -0.053914);
//    Vec3f v3 = Vec3f(-0.0560218,1.32995,0.393053);
//    bbox.minbounds = minimum(minimum(v1,v2),v3);
//    bbox.maxbounds = maximum(maximum(v1,v2),v3);
//    
//    
//    v1 = Vec3f(-0.0560218, 1.32995, 0.393053  );
//    v2 = Vec3f(0.131244, -0.0792365, 0.00260931);
//    v3 = Vec3f(-0.000152177, 1.35546, 0.32272);
//    BoundingBox bbox1;
//    bbox1.minbounds = minimum(minimum(v1,v2),v3);
//    bbox1.maxbounds = maximum(maximum(v1,v2),v3);
//    
//    v1 = Vec3f(0.0752218, 1.25071, 0.395662);
//    v2 = Vec3f(0.05618, -0.00289822, 0.0680239);
//    v3 = Vec3f(-0.000152177, 1.35546, 0.32272);
//    BoundingBox bbox2;
//    bbox2.minbounds = minimum(minimum(v1,v2),v3);
//    bbox2.maxbounds = maximum(maximum(v1,v2),v3);
//
//    bbox = combine(bbox, bbox1);
//    bbox = combine(bbox, bbox2);
//    
//    bbox.print();
    
    // [ create pathtracer ]
    pathtracer = new Pathtracer(window_width,
                                window_height);
    
    // Triangles
    std::vector<Vec3f> vertices;
    if(load_obj("objs/teapot.obj",vertices)) {
        if (!pathtracer->set_triangles(vertices))
            errorm("Couldn't set the triangles.");
    }
    else {
        errorm("Couldn't read the OBJ File.");
    }
    
    // Scene (not triangles)
    pathtracer->set_scene();
    
    // Camera
    pathtracer->set_camera();
    
    // Iterative Render First Bounce
    pathtracer->iterative_render_first_bounce();

    // Display
    initialize(argc, argv);
    
    return 0;
}
// =================

void display() {
    
    iterations++;
    std::clock_t start = std::clock();
    double duration;
//
//    Pixel *f_pixels = pathtracer->render();
//    float* pixels = (float*)f_pixels;
    
    pathtracer->iterative_render();
    pathtracer->reconstruct();
//    exit(1);

    Pixel *f_pixels = pathtracer->get_image();
    float* pixels = (float*)f_pixels;
    
    duration = (std::clock()-start)/(double) CLOCKS_PER_SEC;
    
    std::cout << "iteration: " << iterations << " took "
    << duration << " seconds." << std::endl;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    int width = pathtracer->get_width();
    int height = pathtracer->get_height();
    unsigned char* imageData = new unsigned char[width * height * 3];
    for(int i = 0; i < (width*height*3); i++) {
        imageData[i] = (unsigned char)(clip(pixels[i],0.0,1.0) * 255);
    }
    
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

    //delete
    delete [] imageData;
    
    // Show the texture:
    glBindTexture (GL_TEXTURE_2D, PATH_TRACER_BITMAP);
    glBegin (GL_QUADS);
    glTexCoord2f (0.0, 0.0);
    glVertex3f (0.0, 1.0, 0.0);
    glTexCoord2f (1.0, 0.0);
    glVertex3f (1.0, 1.0, 0.0);
    glTexCoord2f (1.0, 1.0);
    glVertex3f (1.0, 0.0, 0.0);
    glTexCoord2f (0.0, 1.0);
    glVertex3f (0.0, 0.0, 0.0);
    glEnd ();
    
    glColor4f(0.0, 0.0, 0.0, 0.0);
	glColor4f(1.0, 1.0, 1.0, 1.0);

    glutSwapBuffers();
    
    //no redraw when debugging
    if (!DEBUG_MODE) {
        glutPostRedisplay();
    }
}

void keyboard( unsigned char key, int /*x*/, int /*y*/)
{
    switch( key) {
        case('q') :
        {
            Bitmap bm;
            bm.width = pathtracer->get_width();
            bm.height = pathtracer->get_height();
            bm.pixels = pathtracer->get_image();
            std::string path = "../../writeup/images/image"+ std::to_string(pathtracer->get_iterations())
            + ".png";
            save_png_to_file(&bm,path.c_str());
            exit(0);
        }
        case('p'):
        {
            Bitmap bm;
            bm.width = pathtracer->get_width();
            bm.height = pathtracer->get_height();
            bm.pixels = pathtracer->get_image();
            std::string path = "../../writeup/images/image"+ std::to_string(pathtracer->get_iterations())
            + ".png";
            save_png_to_file(&bm,path.c_str());
        }
    }
}

void initialize(int argc, char**argv) {
//    glutDestroyWindow(glutGetWindow());
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("brian's ultimate pathtracer");
    
    initGL();
    
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    
    glutMainLoop();
}

bool initGL() {
    
    // default initialization
    glClearColor( 0.0, 0.0, 0.0, 1.0);
    glDisable( GL_DEPTH_TEST);
    
    // viewport
    glViewport(0, 0, window_width, window_height);
    
    // Set up an orthographic view:
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,1,0,1,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Create a texture for displaying the render:
    glBindTexture(GL_TEXTURE_2D, PATH_TRACER_BITMAP);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Use nearest-neighbor point sampling instead of linear interpolation:
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    // Enable textures:
    glEnable(GL_TEXTURE_2D);
    
    //clGetDeviceInfo(deviceID, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &size, 0);
    
    return true;
}


