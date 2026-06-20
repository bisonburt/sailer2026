/*
    image.c
    Framebuffer + multi-format image writer for the SAILER ray tracer.

    Uses the public-domain stb_image_write single-header library to emit
    PNG / JPEG / BMP / TGA. PPM is written directly so the classic format
    still works without any dependency.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static unsigned char *g_pixels = NULL;  /* RGB888, width*height*3 */
static int g_width = 0;
static int g_height = 0;
static long g_index = 0;                 /* next pixel slot (in components) */

static unsigned char clamp_byte(double v)
{
    int i;
    if (v <= 0.0) return 0;
    if (v >= 1.0) return 255;
    i = (int)(v * 255.0 + 0.5);
    if (i < 0) i = 0;
    if (i > 255) i = 255;
    return (unsigned char)i;
}

int ImageOpen(int width, int height)
{
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "ImageOpen: invalid dimensions %dx%d\n", width, height);
        return 1;
    }
    ImageClose();
    g_width = width;
    g_height = height;
    g_index = 0;
    g_pixels = (unsigned char *)calloc((size_t)width * height * 3, 1);
    if (g_pixels == NULL) {
        fprintf(stderr, "ImageOpen: out of memory for %dx%d image\n", width, height);
        return 1;
    }
    return 0;
}

void ImagePutPixel(rgb_type rgb)
{
    if (g_pixels == NULL) return;
    if (g_index + 3 > (long)g_width * g_height * 3) return; /* guard overrun */
    g_pixels[g_index++] = clamp_byte(rgb.r);
    g_pixels[g_index++] = clamp_byte(rgb.g);
    g_pixels[g_index++] = clamp_byte(rgb.b);
}

/* Lower-case file extension (without the dot), or "" if none. */
static const char *file_ext(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (dot == NULL || dot[1] == '\0') return "";
    return dot + 1;
}

static int ext_is(const char *ext, const char *want)
{
    size_t i;
    for (i = 0; ext[i] && want[i]; i++) {
        if (tolower((unsigned char)ext[i]) != want[i]) return 0;
    }
    return ext[i] == '\0' && want[i] == '\0';
}

static int write_ppm(const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "ImageSave: cannot open %s\n", path);
        return 1;
    }
    fprintf(fp, "P6\n%d %d\n255\n", g_width, g_height);
    fwrite(g_pixels, 3, (size_t)g_width * g_height, fp);
    fclose(fp);
    return 0;
}

int ImageSave(const char *path, int quality)
{
    const char *ext = file_ext(path);
    int ok;

    if (g_pixels == NULL) {
        fprintf(stderr, "ImageSave: no image data\n");
        return 1;
    }
    if (quality < 1) quality = 90;
    if (quality > 100) quality = 100;

    if (ext_is(ext, "png")) {
        ok = stbi_write_png(path, g_width, g_height, 3, g_pixels, g_width * 3);
    } else if (ext_is(ext, "jpg") || ext_is(ext, "jpeg")) {
        ok = stbi_write_jpg(path, g_width, g_height, 3, g_pixels, quality);
    } else if (ext_is(ext, "bmp")) {
        ok = stbi_write_bmp(path, g_width, g_height, 3, g_pixels);
    } else if (ext_is(ext, "tga")) {
        ok = stbi_write_tga(path, g_width, g_height, 3, g_pixels);
    } else if (ext_is(ext, "ppm")) {
        return write_ppm(path);
    } else {
        fprintf(stderr,
            "ImageSave: unknown extension '.%s' (use png, jpg, bmp, tga or ppm)\n",
            ext);
        return 1;
    }

    if (!ok) {
        fprintf(stderr, "ImageSave: failed to write %s\n", path);
        return 1;
    }
    return 0;
}

void ImageClose(void)
{
    free(g_pixels);
    g_pixels = NULL;
    g_width = g_height = 0;
    g_index = 0;
}
