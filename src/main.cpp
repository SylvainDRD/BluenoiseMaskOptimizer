#include <chrono>
#include <thread>
#include <iomanip>

#include <utils.hpp>
#include <display.hpp>
#include <optimizer.hpp>

#include <GLFW/glfw3.h>


using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

bool handleArgs(int &maskSize, int &dimension, int argc, char **argv);

int main(int argc, char **argv) {
    int maskSize, dimension;

    if(!handleArgs(maskSize, dimension, argc, argv)) {
        ERROR << "Invalid arguments, possible usages :\n"
                 "1) ./BluenoiseMaskOptimizer\n"
                 "2) ./BluenoiseMaskOptimizer MaskSize Dimension\n"
                 "Note: MaskSize should be a power of two such that 2^7 <= MaskSize <= 2^10 and 1 <= Dimension <= 20"
              << std::endl;

        return INVALID_ARGUMENTS;
    }

    // GLFW initialization
    if(!glfwInit()) {
        ERROR << "There was an issue during the initialization of GLFW" << std::endl;

        return GLFW_INIT_ERROR;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(maskSize, maskSize, "BMO", nullptr, nullptr);

    if(!window) {
        ERROR << "There was an issue during the initialization of the GLFW window" << std::endl;

        glfwTerminate();
        return GLFW_WINDOW_ERROR;
    }

    glfwMakeContextCurrent(window);

    // OpenGL initialization
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        ERROR << "There was an issue loading the OpenGL function: make sure your GPU is compatible with "
                 "OpenGL 4.3."
              << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();
        return GL_LOAD_ERROR;
    }

    // Check the GPU capabilities
    GLint texture3DSize;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &texture3DSize);
    if(maskSize > texture3DSize) {
        ERROR << "Your OpenGL implementation only support 3D textures with size " << texture3DSize
              << ": please reduce the size of the mask." << std::endl;

        return GL_TEXTURE_SIZE_ERROR;
    }

    Optimizer optimizer(maskSize, dimension);
    Display display(dimension, optimizer.maskTexture());
    LOG << "Initialization complete. Beginning the optimization." << std::endl;

    auto start = steady_clock::now();
    while(!glfwWindowShouldClose(window)) {
        optimizer.run();
        glFinish();

        if(duration_cast<milliseconds>(steady_clock::now() - start).count() > 100) {
            display.draw();
            LOG << "Accepted permutations: " << std::setw(6) << optimizer.acceptedSwapCount() << '\r' << std::flush;

            glfwSwapBuffers(window);
            glfwPollEvents();

            start = steady_clock::now();
        }
    }

    LOG << "\n";
    LOG << "Exporting the mask and cleaning up before exiting..." << std::endl;

    // Save the last mask
    optimizer.exportMaskAsPPM(PROJECT_ROOT "mask.ppm");
    optimizer.exportMaskAsHeader(PROJECT_ROOT "mask.h");

    // Cleanup
    display.freeGLRessources();
    optimizer.freeGLRessources();

    glfwDestroyWindow(window);
    glfwTerminate();

    return SUCCESS;
}


bool handleArgs(int &maskSize, int &dimension, int argc, char **argv) {
    switch(argc) {
    case 1:
        maskSize = 128;
        dimension = 1;
        return true;

    case 3:
        maskSize = std::atoi(argv[1]);
        dimension = std::atoi(argv[2]);

        // Round up the size to the next power of two if it is not already one
        if(maskSize & (maskSize - 1)) {
            int32_t s = maskSize;

            --s;
            s |= s >> 1;
            s |= s >> 2;
            s |= s >> 4;
            s |= s >> 8;
            s |= s >> 16;

            maskSize = s + 1;
        }

        return (maskSize >= 128 && maskSize <= 1024) && (dimension >= 1 && dimension <= 20);

    default:
        return false;
    }
}
