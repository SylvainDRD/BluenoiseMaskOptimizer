# Bluenoise Mask Optimizer

## What is this ? 

Hello there ! This application's purpose is to generate tileable bluenoise masks with up to 20 dimensions on the GPU. The function minimized to produce those masks comes from the research paper [*Blue-noise Dithered Sampling*](https://www.arnoldrenderer.com/research/dither_abstract.pdf), Iliyan Georgiev & Marcos Fajardo (2016). 

Note that the quality that can be achieved using that optimization in 20D is very limited. In practice, trying to go higher than 10 to 12 dimensions will yield poor quality bluenoises.


## Requirements

The application has two requirements:
 - OpenGL 4.3 for compute shaders
 - CMake 3.2 for makefiles generation

It also uses [GLFW](https://github.com/glfw/glfw) to generate an OpenGL context and [GLAD](https://github.com/Dav1dde/glad) to load the OpenGL functions. GLFW is included in the repository as a submodule, so make sure you get it using ```git submodule update --init``` after cloning the repository if you did not clone using the ```--recursive``` option in the first place.


## Usage

The application expects either none or two parameters. In the first case the optimization process starts for a 128 by 128 1D mask. In the second case you can provide a power of two for the size and the dimension as parameters (e.g. ```./BluenoiseMaskOptimizer 128 3``` for a 128 by 128 3D mask). The power of two for the mask size must be in [128, 1024]. Note that the size will be rounded up to the next power of two if it is not already one. As I previously said, the maximum dimension is 20.

On launch, the optimization process starts automatically and the current state of the mask is updated every 100 ms or so in the GLFW window (obviously only the first 3 channels are displayed). As the permutations quickly get hard to visualize, the total number of permutations that were applied to the original whitenoise mask is constantly updated in the terminal.

When the window is closed, the optimization stops and the last iteration of the mask is saved as a header and a PPM file at the root of the project (i.e. mask.h/mask.ppm).


## Sample masks
Here is the kind of mask you can expect from that optimization in dimension 1 and 3:

<p align="center">
    <img src="https://i.imgur.com/hwtUefg.png" alt="A 1D bluenoise mask">
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <img src="https://i.imgur.com/uzaAIf8.png alt="A 3D bluenoise mask">
</p>
