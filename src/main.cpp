

// Resize 3-channel PNG for all images in folder

#include <cstring>
#include <iostream>
#include <vector>
#include <tuple>
#include <dirent.h>

#include <npp.h>
#include <nppdefs.h>

#include <FreeImage.h>
#include <Exceptions.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>
#include <helper_cuda.h>

#include <functions.h>

// selection of algorithms for resizing and composition
// check https://docs.nvidia.com/cuda/archive/10.2/npp/group__typedefs__npp.html
const NppiInterpolationMode INTERPOLATION_MODE = NPPI_INTER_CUBIC;
const NppiAlphaOp ALPHA_BLENDING_OPERATION = NPPI_OP_ALPHA_OVER;



/// @brief Loads an image from a file and converts it to 32-bit RGBA format.
/// @param file_path The path to the input image file.
/// @param alpha The custom alpha value to set for all pixels (from 0 to 1).
/// @param npp_image_host reference to an NPP ImageCPU_8u_C4 object (in/out).
/// @details Uses FreeImage to load the image, sets a custom alpha value,
///          and converts it to an NPP-compatible image.
void loadImage(
    const std::string file_path, const double alpha,
    npp::ImageCPU_8u_C4 &npp_image_host)
{
    FIBITMAP *p_bitmap_start = FreeImage_Load(FIF_PNG, file_path.c_str(), PNG_DEFAULT);
    if (!p_bitmap_start)
        std::cerr << "Failed to load image!" << std::endl;
    
    // forcing it to be 32 bits
    FIBITMAP *p_bitmap = FreeImage_ConvertTo32Bits(p_bitmap_start);
    setAlpha(p_bitmap, alpha);

    // Convert the FreeImage bitmap to an NPP image
    npp::ImageCPU_8u_C4 npp_image;
    convertFreeImageToNppImage<npp::ImageCPU_8u_C4>(p_bitmap, npp_image);

    FreeImage_Unload(p_bitmap_start);
    FreeImage_Unload(p_bitmap);

    npp_image_host.swap(npp_image);
}


/// @brief Resize an image.
/// @param npp_image_host Image to resize in CPU, reference to an NPP ImageCPU_8u_C4 object.
/// @param width Target width.
/// @param height Target height.
/// @param nppImageDevice Resized image in GPU, reference to an NPP ImageNPP_8u_C4 object (in/out).
/// @details Uses CUBIC interpolation.
void resizeImage(
    npp::ImageCPU_8u_C4 &npp_image_host,
    const uint width, const uint height,
    npp::ImageNPP_8u_C4 &nppImageDevice)
{

    const int widthSrc = npp_image_host.width();
    const int heightSrc = npp_image_host.height();
    NppiSize inSize = {widthSrc, heightSrc};
    NppiRect inROI = {0, 0, widthSrc, heightSrc};
    NppiPoint inOffset = {0, 0};

    // allocate device image of appropriately size
    const double nXFactor = static_cast<double>(width) / widthSrc;
    const double nYFactor = static_cast<double>(height) / heightSrc;
    const double nXShift = 0.0;
    const double nYShift = 0.0;

    NppiRect dstRect;
    nppiGetResizeRect(
        inROI, &dstRect, nXFactor, nYFactor, nXShift, nYShift,
        INTERPOLATION_MODE
        );
    npp::ImageNPP_8u_C4 nppImageDeviceIn(npp_image_host);
    npp::ImageNPP_8u_C4 nppImageDeviceResized(dstRect.width, dstRect.height);
    // run resize filter
    NPP_CHECK_NPP(
        nppiResizeSqrPixel_8u_C4R(
            nppImageDeviceIn.data(), inSize, nppImageDeviceIn.pitch(), inROI,
            nppImageDeviceResized.data(), nppImageDeviceResized.pitch(), dstRect,
            nXFactor, nYFactor, nXShift, nYShift,
            INTERPOLATION_MODE));

    cudaFree(&nppImageDeviceIn);

    // swap the user given image with our result image, effecively
    // moving our newly loaded image data into the user provided shell
    nppImageDevice.swap(nppImageDeviceResized);
}



/// @brief Set the arguments based on call.
std::tuple<std::string,uint,uint,double,std::string> getArguments(int argc, char *argv[])
{

    // default values
    std::string inputPath = "./data/flags";
    std::string outputPath = "./results/";
    // using the most common flag ratio
    const double ratio = 2.0 / 3.0;
    int width = 1000;
    int height = ratio * width;
    double alpha = 0.1;

    char *buffer;
    if (checkCmdLineFlag(argc, (const char **)argv, "input")) {
        getCmdLineArgumentString(argc, (const char **)argv, "input", &buffer);
        inputPath = buffer;
    }
    if (checkCmdLineFlag(argc, (const char **)argv, "width")) {
        getCmdLineArgumentString(argc, (const char **)argv, "width", &buffer);
        width = std::atoi(buffer);
    }
    if (checkCmdLineFlag(argc, (const char **)argv, "height")) {
        getCmdLineArgumentString(argc, (const char **)argv, "height", &buffer);
        height = std::atoi(buffer);
    }
    if (checkCmdLineFlag(argc, (const char **)argv, "alpha"))
    {
        getCmdLineArgumentString(argc, (const char **)argv, "alpha", &buffer);
        alpha = std::stod(buffer);
    }
    if (checkCmdLineFlag(argc, (const char **)argv, "output")) {
        getCmdLineArgumentString(argc, (const char **)argv, "output", &buffer);
        outputPath = buffer;
    }

    std::cout << std::endl;
    std::cout << " input folder: " << inputPath << std::endl;
    std::cout << " width: " << width << std::endl;
    std::cout << " height: " << height << std::endl;
    std::cout << " alpha: " << alpha << std::endl;
    std::cout << " output folder: " << outputPath << std::endl;
    std::cout << std::endl;

    return std::make_tuple(inputPath, width, height, alpha, outputPath);
}


/// @brief Get list of png files in folder.
/// @param Path to folder.
/// @return Vector with paths to png files in folder.
std::vector<std::string> getFiles(const std::string inputPath)
{
    std::vector<std::string> images;
    DIR *dir = opendir(inputPath.c_str());
    if (dir == nullptr)
    {
        std::cerr << "Error: Could not open directory " << inputPath << std::endl;
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        if (entry->d_type == DT_REG) {
            size_t dotPos = name.find_last_of(".");
            std::string ext = name.substr(dotPos + 1);
            std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower);
            // only include .png files
            if (ext == "png") {
                images.push_back(inputPath + "/" + name);
            }
        }
    }
    closedir(dir);
    // TODO: make it random or ask user it want to sort
    std::sort(images.begin(), images.end());

    return images;
}


int main(int argc, char *argv[])
{

    std::string input_path, output_path;
    int width, height;
    double alpha;

    std::tie(input_path, width, height, alpha, output_path) = getArguments(argc, argv);
    auto images = getFiles(input_path);
    const Npp8u number_images = static_cast<Npp8u>(images.size());

    std::cout << " number of images: " << (int)number_images << std::endl;

    NppiSize oSizeROI = {width, height};

    // this loop could be eventually replaced by a CUDA kernel, but you would 
    // not have the control of the order of the combinations and could not use
    // NPP functions (host code only)
    npp::ImageNPP_8u_C4 last_image;
    size_t i = 0;
    for (auto file : images)
    {

        npp::ImageCPU_8u_C4 nppImageHoStTransparent;
        loadImage(file, alpha, nppImageHoStTransparent);

        npp::ImageNPP_8u_C4 nppImageDeviceTransparent;
        resizeImage(nppImageHoStTransparent, width, height, nppImageDeviceTransparent);


        npp::ImageCPU_8u_C4 npp_image_host(nppImageDeviceTransparent.size());
        nppImageDeviceTransparent.copyTo(npp_image_host.data(), npp_image_host.pitch());
        saveImage<npp::ImageCPU_8u_C4>(file, output_path, npp_image_host);

        // add to the latest image
        npp::ImageNPP_8u_C4 nppImageDeviceCombined(nppImageDeviceTransparent.size());
        if (i > 0) {
            NPP_CHECK_NPP(
                nppiAlphaComp_8u_AC4R(
                    nppImageDeviceTransparent.data(), nppImageDeviceTransparent.pitch(),
                    last_image.data(), last_image.pitch(),
                    nppImageDeviceCombined.data(), nppImageDeviceCombined.pitch(),
                    oSizeROI, ALPHA_BLENDING_OPERATION
                    )
                );
            // put image on host
            nppImageDeviceCombined.copyTo(npp_image_host.data(), npp_image_host.pitch());
            saveImage<npp::ImageCPU_8u_C4>(
                "" + std::to_string(i) + ".png", output_path, npp_image_host);
        }

        last_image = nppImageDeviceCombined;
        i++;

        cudaFree(&nppImageDeviceTransparent);
    }

    return EXIT_SUCCESS;
}
