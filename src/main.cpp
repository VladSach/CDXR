#include "Renderer.h"

i32 main()
{
    chaos::Renderer renderer;
    renderer.init();

    renderer.draw();

    // renderer.deinit();

    return 0;
}
