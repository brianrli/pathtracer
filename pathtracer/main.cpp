// program entrance
#include "opencl_manager.hpp"
#include "png_writer.hpp"
#include "pathtracer.hpp"

int main(int argc, const char * argv[]) {

    // testing
    int width = 720;
    int height = 480;

    float camera_x;
    float camera_y;
    float camera_z;

    // for each pixel
        // compute viewing ray
        // find first object hit by ray and its surface normal n
        // set pixel color to value based on material, light, and n

    //create pathtracer
    Pathtracer pathtracer;
    Pixel pixel;
    Color color;
    
    // test opencl capabilities
    OpenCL_Manager manager;
    manager.initialize();
    manager.load_kernel("pathtracer_kernel.cl", "pathtracer_kernel");
    manager.execute_kernel();
    
    // test image writing capabilities
    {
        Bitmap bmp;
        bmp.width = 720;
        bmp.height = 480;

        // c style because I have no idea what I'm doing
        bmp.pixels = (Pixel*) calloc(sizeof(Pixel),bmp. width * bmp.height);
        for (int i = 0; i < bmp.width * bmp.height; i++) {
            bmp.pixels[i].red = 100;
            bmp.pixels[i].green = 255;
            bmp.pixels[i].blue = 100;
        }
        
        save_png_to_file(&bmp,"test.png");
    }
    return 0;
}




