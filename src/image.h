/*
    image.h
    Modern output stage for the SAILER ray tracer.

    Replaces the original PPM-only writer (writeppm.c). Pixels are accumulated
    into an in-memory RGB framebuffer and written out in a format chosen by the
    output file's extension: .png, .jpg/.jpeg, .bmp, .tga or .ppm.
*/

#ifndef IMAGE_H
#define IMAGE_H

#include "structs.h"   /* rgb_type */

/* Allocate the framebuffer. Returns 0 on success, non-zero on failure. */
int ImageOpen(int width, int height);

/* Append one pixel (components in the range 0.0 .. 1.0), left-to-right,
   top-to-bottom, exactly as the original WritePPM was called. */
void ImagePutPixel(rgb_type rgb);

/* Write the framebuffer to 'path'. Format is selected from the extension.
   For JPEG, 'quality' (1..100) is used; ignored for other formats.
   Returns 0 on success, non-zero on failure. */
int ImageSave(const char *path, int quality);

/* Release the framebuffer. */
void ImageClose(void);

#endif /* IMAGE_H */
