

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <cstring>
#include <iostream>
#include <algorithm>

#include <npp.h>

#include <FreeImage.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>

/// @brief Converts a FreeImage bitmap to an NVIDIA NPP-compatible image.
/// @tparam T The type of the NPP image (e.g., npp::ImageCPU_8u_C4).
/// @param bitmap Pointer to the loaded FreeImage bitmap (FIBITMAP).
/// @param out_image Reference to the output NPP image object.
template <typename T>
void convertFreeImageToNppImage(
    FIBITMAP *bitmap, T &out_image)
{
    // Get image dimensions and pitch from the FreeImage bitmap
    int width = FreeImage_GetWidth(bitmap);
    int height = FreeImage_GetHeight(bitmap);
    unsigned int pitch = FreeImage_GetPitch(bitmap);
    BYTE *p_bits = FreeImage_GetBits(bitmap);

    // Copy data from FreeImage bitmap to npp ImageCPU
    T npp_image(width, height);
    Npp8u *npp_data = npp_image.data();
    for (int y = 0; y < height; ++y)
    {
        // source row pointer in FreeImage bitmap
        const BYTE *row_src = p_bits + y * pitch;
        // destination row pointer in NPP image
        Npp8u *dst_row = npp_data + y * pitch;
        std::memcpy(dst_row, row_src, pitch);
    }
    // swap the user given image with our result image
    out_image.swap(npp_image);
}


/// @brief Converts a NVIDIA NPP-compatible image to a FreeImage bitmap.
/// @tparam T The type of the NPP image (e.g., npp::ImageCPU_8u_C4).
/// @param nppImage Reference to the input NPP image object.
/// @return Pointer to the loaded FreeImage bitmap (FIBITMAP).
template <typename T>
FIBITMAP* convertNppImageToFreeImage(const T &nppImage)
{
    int width = nppImage.width();
    int height = nppImage.height();
    int pitch = nppImage.pitch();
    const uint channels = pitch / width;

    // Create a FreeImage bitmap (RGB)
    FIBITMAP *bitmap = FreeImage_Allocate(width, height, channels*8);
    if (!bitmap)
    {
        std::cerr << "Failed to allocate FIBITMAP!" << std::endl;
        return nullptr;
    }

    BYTE *p_bits = FreeImage_GetBits(bitmap);
    // copy data from npp::ImageCPU_8u_C4 to FreeImage bitmap
    const Npp8u *npp_data = nppImage.data();
    for (int y = 0; y < height; ++y)
    {
        // source row pointer in NPP image
        const Npp8u *row_src = npp_data + y * pitch;
        // destination row pointer in FreeImage bitmap
        BYTE *dst_row = p_bits + y * pitch;
        memcpy(dst_row, row_src, pitch);
    }

    return bitmap;
}


/// @brief Sets opacity for all pixels in an RGBA FreeImage bitmap.
/// @param bitmap Pointer to the FreeImage bitmap.
/// @param alpha Alpha value for all pixels (from 0.0 to 1.0).
void setAlpha(FIBITMAP *bitmap, const double alpha)
{
    BYTE opacity = std::min(alpha, (double)1.0) * 255;
    unsigned int width = FreeImage_GetWidth(bitmap);
    unsigned int height = FreeImage_GetHeight(bitmap);

    // Update the alpha channel with the custom opacity value
    // TODO: replace by call to FreeImage_SetTransparencyTable?
    for (unsigned int y = 0; y < height; y++)
    {
        BYTE *scanLine = FreeImage_GetScanLine(bitmap, y);
        for (unsigned int x = 0; x < width; x++)
        {
            scanLine[FI_RGBA_ALPHA] = opacity;
            // 4 bytes per pixel in RGBA
            scanLine += 4;
        }
    }
}

/// @brief Saves NPP image to a file in PNG format.
/// @tparam T type of the NPP image stored in host memory (e.g., npp::ImageCPU_8u_C3).
/// @param file The name of the file to save.
/// @param folder The destination folder path.
/// @param npp_image_host The NPP image object in host memory to be saved.
template <typename T>
void saveImage(const std::string file, const std::string folder, T &npp_image_host)
{
    // convert the NPP image to FreeImage
    FIBITMAP *bitmap = convertNppImageToFreeImage<T>(npp_image_host);
    std::string file_path_out = folder;
    const size_t pos = file.find_last_of("/\\");
    const std::string file_name = (pos != std::string::npos) ? file.substr(pos + 1) : file;
    file_path_out += file_name;

    if (!FreeImage_Save(FIF_PNG, bitmap, file_path_out.c_str(), PNG_DEFAULT))
        std::cerr << "Failed to save image!" << std::endl;
    
    FreeImage_Unload(bitmap);
}


#endif //  FUNCTIONS_H