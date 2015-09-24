// program entrance
#include "opencl_manager.hpp"
#include "png_writer.hpp"
#include "pathtracer.hpp"

int main(int argc, const char * argv[]) {

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




