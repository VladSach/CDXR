#include "Renderer.h"

i32 main()
{
    chaos::Renderer renderer;
    renderer.init();

    renderer.set_clear_color(.2f);
    renderer.draw();
    renderer.set_clear_color(.8f);
    renderer.draw();

    // renderer.deinit();

    return 0;
}
