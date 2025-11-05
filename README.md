# 

base on FreeImage and NPP (NVIDIA 2D Image And Signal Performance Primitives).
First it convert all images to RGBA for an uniform alpha and resizes them.
Them, all the images are combined.

The used interpolation and blending operation are hard-coded in `main.cpp` as
`NPPI_INTER_CUBIC` and `NPPI_OP_ALPHA_OVER`.

From https://github.com/hampusborgos/country-flags.git

## How to

### Build

Header files can be obtained by running `make getCode` or `make get` (also getting the data if not found), or downloaded directly (last november/2025):

- CUDA samples: https://github.com/NVIDIA/cuda-samples
- FreeImage: http://downloads.sourceforge.net/freeimage/FreeImage3180.zip

Using CUDA 11.3 with C++11


### Run
