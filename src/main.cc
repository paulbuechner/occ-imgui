#include "occ_imgui/occ-imgui-glfw-occt-view.h"

int main(int, char**)
{
    try
    {
        GlfwOcctView anApp;
        anApp.run();
    }
    catch (const std::runtime_error& theError)
    {
        std::cerr << theError.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
