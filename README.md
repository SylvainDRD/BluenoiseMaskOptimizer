# Bluenoise Mask Optimizer

## What is this ? 

Hello there ! This application's purpose is to generate tileable bluenoise masks with up to 20 dimensions on the GPU. 
The original minimization process used to produce those masks comes from the research paper [*Blue-noise Dithered Sampling*](https://www.arnoldrenderer.com/research/dither_abstract.pdf), Iliyan Georgiev & Marcos Fajardo (2016). 
However, the actual loss function minimized in this application is almost axactly the same as the one introduced in [*A Low-Discrepancy Sampler that Distributes Monte Carlo Errors as a Blue Noise in Screen Space*](https://belcour.github.io/blog/research/publication/2019/06/17/sampling-bluenoise.html), Heitz et al. (2019).


## Requirements

The application has two requirements:
 - OpenGL 4.3 for compute shaders
 - CMake 3.2 for makefiles generation

It also uses [GLFW](https://github.com/glfw/glfw) to generate an OpenGL context and [GLAD](https://github.com/Dav1dde/glad) to load the OpenGL functions. 
GLFW is included in the repository as a submodule, so make sure you get it using ```git submodule update --init``` after cloning the repository if you did not clone using the ```--recursive``` option in the first place.


## Build instructions

# On Windows (with a Visual Studio):

On any version on VS that supports CMake, just open the project directory with the IDE it's really that simple (tested on VS 2019).

# On Linux:

After cloning the repository and its dependencies (see the *Requirements* section), simply do the following in terminal while at the root of the project:
```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make 
```

And you should be good to go!


## Usage

The application expects either none or two parameters. In the first case the optimization process starts for a 128 by 128 1D mask. In the second case you can provide a power of two for the size and the dimension as parameters (e.g. ```./BluenoiseMaskOptimizer 128 3``` for a 128 by 128 3D mask). The power of two for the mask size must be in [128, 1024]. 
Note that the size will be rounded up to the next power of two if it is not already one. As previously stated, the maximum dimension is 20.

On launch, the optimization process starts automatically and the current state of the mask is updated every 100 ms or so in the preview (a maximum of 3 channels are displayed). As the permutations quickly get hard to visualize, the total number of permutations that were applied to the original whitenoise mask is constantly updated in the terminal.

When the window is closed, the optimization stops and the latest version of the mask is saved as a header and a PPM file at the root of the project (i.e. mask.h/mask.ppm).


## Sample masks
Here is the kind of mask you can expect from that optimization in dimension 1 and 3:

<p align="center">
    <img src="https://i.imgur.com/hwtUefg.png" alt="A 1D bluenoise mask">
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <img src="https://i.imgur.com/uzaAIf8.png alt="A 3D bluenoise mask">
</p>
