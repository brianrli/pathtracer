// ===[A C K N O W L E D G E M E N T S]===
//The GUI code is taken from Peter and Karl's GPU Pathtracer
//https://github.com/peterkutz/GPUPathTracer

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "pathtracer.hpp"
#include "opencl_manager.hpp"
#include "png_writer.hpp"

#include <ctime>

#define PATH_TRACER_BITMAP 13

Pathtracer *pathtracer = NULL;
int window_height = 720;
int window_width = 720;
int size = window_height * window_width;
int iterations = 0;

//forward declarations
void initialize(int argc, char**argv);
bool initGL();

float clip(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}

// ===[ M A I N ]===
int main(int argc, char **argv) {
    
    //create pathtracer
    pathtracer = new Pathtracer();

    pathtracer->set_scene();
    pathtracer->set_camera();
    
    size = window_width * window_height;
    
    //display
    initialize(argc, argv);
    
    return 0;
}

void display() {
    
    iterations++;
    std::clock_t start = std::clock();
    double duration;
    
    Pixel *f_pixels = pathtracer->fake_render();
    float* pixels = (float*)f_pixels;
    
    duration = (std::clock()-start)/(double) CLOCKS_PER_SEC;
//    
//    std::cout << "iteration: " << iterations << " took "
//    << duration << " seconds." << std::endl;
    
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
    
	glColor4f(1.0, 1.0, 1.0, 1.0);
    
    glutSwapBuffers();
    
    //no redraw when debugging
    glutPostRedisplay();
}

void initialize(int argc, char**argv) {
    
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("brian's pathtracer");
    
    initGL();
    
    glutDisplayFunc(display);
    
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
    
    return true;
}


