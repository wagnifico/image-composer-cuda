

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
/// @param pBitmap Pointer to the loaded FreeImage bitmap (FIBITMAP).
/// @param outImage Reference to the output NPP image object.
template <typename T>
void convertFreeImageToNppImage(
    FIBITMAP *pBitmap, T &outImage)
{
    // Get image dimensions and pitch from the FreeImage bitmap
    int width = FreeImage_GetWidth(pBitmap);
    int height = FreeImage_GetHeight(pBitmap);
    unsigned int pitch = FreeImage_GetPitch(pBitmap);
    BYTE *pFreeImageBits = FreeImage_GetBits(pBitmap);

    // Copy data from FreeImage bitmap to npp ImageCPU
    T nppImage(width, height);
    Npp8u *pNppData = nppImage.data(); // Pointer to NPP image data
    for (int y = 0; y < height; ++y)
    {
        // Source row pointer in FreeImage bitmap
        const BYTE *pSourceRow = pFreeImageBits + y * pitch;
        Npp8u *pDestRow = pNppData + y * pitch;
        std::memcpy(pDestRow, pSourceRow, pitch);
    }
    // swap the user given image with our result image, effecively
    // moving our newly loaded image data into the user provided shell
    outImage.swap(nppImage);
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
    FIBITMAP *pBitmap = FreeImage_Allocate(width, height, channels*8);
    if (!pBitmap)
    {
        std::cerr << "Failed to allocate FIBITMAP!" << std::endl;
        return nullptr;
    }

    BYTE *pFreeImageBits = FreeImage_GetBits(pBitmap);
    // Copy data from npp::ImageCPU_8u_C4 to FreeImage bitmap
    const Npp8u *pNppData = nppImage.data(); // Pointer to NPP image data
    for (int y = 0; y < height; ++y)
    {
        // Source row pointer in NPP image
        const Npp8u *pSourceRow = pNppData + y * pitch;
        // Destination row pointer in FreeImage bitmap
        BYTE *pDestRow = pFreeImageBits + y * pitch;
        memcpy(pDestRow, pSourceRow, pitch);
    }

    return pBitmap;
}


/// @brief Sets opacity for all pixels in an RGBA FreeImage bitmap.
/// @param pBitmap Pointer to the FreeImage bitmap.
/// @param alpha Alpha value for all pixels (from 0.0 to 1.0).
void setAlpha(FIBITMAP *pBitmap, const double alpha)
{
    BYTE opacity = std::min(alpha, (double)1.0) * 255;
    unsigned int width = FreeImage_GetWidth(pBitmap);
    unsigned int height = FreeImage_GetHeight(pBitmap);

    // Update the alpha channel with the custom opacity value
    // TODO: replace by call to FreeImage_SetTransparencyTable?
    for (unsigned int y = 0; y < height; y++)
    {
        BYTE *scanLine = FreeImage_GetScanLine(pBitmap, y);
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
/// @param nppImageHost The NPP image object in host memory to be saved.
template <typename T>
void saveImage(const std::string file, const std::string folder, T &nppImageHost)
{
    // convert the NPP image to FreeImage
    FIBITMAP *pBitmap = convertNppImageToFreeImage<T>(nppImageHost);
    std::string filePathOut = folder;
    const size_t pos = file.find_last_of("/\\");
    const std::string fileName = (pos != std::string::npos) ? file.substr(pos + 1) : file;
    filePathOut += fileName;

    if (!FreeImage_Save(FIF_PNG, pBitmap, filePathOut.c_str(), PNG_DEFAULT))
        std::cerr << "Failed to save image!" << std::endl;
    
    FreeImage_Unload(pBitmap);
}


#endif //  FUNCTIONS_H