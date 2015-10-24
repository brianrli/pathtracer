// program entrance
#include "opencl_manager.hpp"
#include "png_writer.hpp"
#include "pathtracer.hpp"

#include <ctime>

int main(int argc, const char * argv[]) {

    std::clock_t start;
    double duration;
    
    start = std::clock();
    
    //create pathtracer
    Pathtracer* pathtracer = new Pathtracer();

    pathtracer->set_scene();
    pathtracer->set_camera();

    Bitmap* bmp = pathtracer->fake_render();

    save_png_to_file(bmp,"third_render.png");
    
    duration = (std::clock()-start)/(double) CLOCKS_PER_SEC;
    std::cout << "Render took " << duration << " seconds." << std::endl;
    
    return 0;
}




