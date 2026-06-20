/*
    main.c
    Command-line front-end for the SAILER ray tracer.

    Usage:
        ray <scene.json> [-o output.png] [-q quality]

    The output format is inferred from the output file extension
    (png, jpg/jpeg, bmp, tga, ppm). If -o is omitted, the output defaults
    to the scene file's base name with a .png extension.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* defined in raymain.c */
int ProcessImage(const char *infile, const char *outfile, int quality);

static void usage(const char *prog)
{
    printf("SAILER ray tracer (2026 JSON edition)\n");
    printf("usage: %s <scene.json> [-o output.png] [-q quality]\n", prog);
    printf("\n");
    printf("  <scene.json>     JSON scene description (required)\n");
    printf("  -o, --output F   output image; format from extension\n");
    printf("                   (.png .jpg .jpeg .bmp .tga .ppm). default: <scene>.png\n");
    printf("  -q, --quality N  JPEG quality 1..100 (default 90)\n");
    printf("  -h, --help       this help\n");
}

/* Derive "<base>.png" from an input path, e.g. scenes/cone.json -> cone.png */
static void default_output(const char *infile, char *out, size_t outsz)
{
    const char *slash = strrchr(infile, '/');
    const char *base = slash ? slash + 1 : infile;
    const char *dot = strrchr(base, '.');
    size_t n = dot ? (size_t)(dot - base) : strlen(base);
    if (n >= outsz - 5) n = outsz - 5;
    memcpy(out, base, n);
    out[n] = '\0';
    strncat(out, ".png", outsz - strlen(out) - 1);
}

int main(int argc, char *argv[])
{
    const char *infile = NULL;
    const char *outfile = NULL;
    char outbuf[1024];
    int quality = 90;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (++i >= argc) { fprintf(stderr, "error: -o needs an argument\n"); return 1; }
            outfile = argv[i];
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quality") == 0) {
            if (++i >= argc) { fprintf(stderr, "error: -q needs an argument\n"); return 1; }
            quality = atoi(argv[i]);
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        } else if (infile == NULL) {
            infile = argv[i];
        } else {
            fprintf(stderr, "error: unexpected argument %s\n", argv[i]);
            return 1;
        }
    }

    if (infile == NULL) {
        usage(argv[0]);
        return 1;
    }

    if (outfile == NULL) {
        default_output(infile, outbuf, sizeof(outbuf));
        outfile = outbuf;
    }

    printf("SAILER: rendering %s -> %s\n", infile, outfile);
    return ProcessImage(infile, outfile, quality);
}
