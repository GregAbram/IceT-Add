/* -*- c -*- *****************************************************************
** Copyright (C) 2011 Sandia Corporation
** Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
** the U.S. Government retains certain rights in this software.
**
** This source code is released under the New BSD License.
**
** This test checks the behavior of various ways to copy blocks of pixels
** in sparse images.
*****************************************************************************/

#include "test_codes.h"
#include "test-util.h"

#include <IceTDevImage.h>

#include <stdlib.h>
#include <stdio.h>

/* Encode image position in color. */
#define ACTIVE_COLOR(x, y) \
    ((((x) & 0xFFFF) | 0x8000) | ((((y) & 0xFFFF) | 0x8000) << 16))

/* Fills the given image to have data in the lower triangle like this:
 *
 * +-------------+
 * |  \          |
 * |    \        |
 * |      \      |
 * |        \    |
 * |          \  |
 * +-------------+
 *
 * Where the lower half is filled with data and the upper half is background. */
static void LowerTriangleImage(IceTImage image)
{
    IceTUInt *data = icetImageGetColorui(image);
    IceTSizeType width = icetImageGetWidth(image);
    IceTSizeType height = icetImageGetHeight(image);
    IceTSizeType x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (x < (height-y)) {
                data[0] = ACTIVE_COLOR(x, y);
            } else {
                data[0] = 0;
            }
            data++;
        }
    }
}

/* Fills the given image to have data in the upper triangle like this:
 *
 * +-------------+
 * |  \          |
 * |    \        |
 * |      \      |
 * |        \    |
 * |          \  |
 * +-------------+
 *
 * Where the upper half is filled with data and the upper half is background. */
static void UpperTriangleImage(IceTImage image)
{
    IceTUInt *data = icetImageGetColorui(image);
    IceTSizeType width = icetImageGetWidth(image);
    IceTSizeType height = icetImageGetHeight(image);
    IceTSizeType x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if ((height-y) < x) {
                data[0] = ACTIVE_COLOR(x, y);
            } else {
                data[0] = 0;
            }
            data++;
        }
    }
}

static int CompareSparseImages(const IceTSparseImage image1,
                               const IceTSparseImage image2)
{
    /* This is a little bit hacky, but the sparse image internals are never
     * exposed. */
    IceTUInt *buffer1;
    IceTUInt *buffer2;
    IceTSizeType buffer_size1;
    IceTSizeType buffer_size2;
    IceTSizeType entries;
    IceTSizeType i;

    icetSparseImagePackageForSend(image1, (IceTVoid**)&buffer1, &buffer_size1);
    icetSparseImagePackageForSend(image2, (IceTVoid**)&buffer2, &buffer_size2);

    if (buffer_size1 != buffer_size2) {
        printf("Buffer sizes do not match: %d vs %d!\n",
               buffer_size1, buffer_size2);
        return TEST_FAILED;
    }

    entries = buffer_size1/sizeof(IceTUInt);
    for (i = 0; i < entries; i++) {
        if (buffer1[i] != buffer2[i]) {
            printf("Buffer mismatch at uint %d\n", i);
            printf("0x%x vs 0x%x\n", buffer1[i], buffer2[i]);
            return TEST_FAILED;
        }
    }

    return TEST_PASSED;
}

static int TrySparseImageCopyPixels(const IceTImage image,
                                    IceTSizeType start,
                                    IceTSizeType end)
{
    IceTVoid *full_sparse_buffer;
    IceTSparseImage full_sparse;
    IceTVoid *compress_sub_buffer;
    IceTSparseImage compress_sub;
    IceTVoid *sparse_copy_buffer;
    IceTSparseImage sparse_copy;

    IceTSizeType width = icetImageGetWidth(image);
    IceTSizeType height = icetImageGetHeight(image);
    IceTSizeType sub_size = end - start;

    int result;

    printf("Trying sparse image copy from %d to %d\n", start, end);

    full_sparse_buffer = malloc(icetSparseImageBufferSize(width, height));
    full_sparse = icetSparseImageAssignBuffer(full_sparse_buffer,width,height);

    compress_sub_buffer = malloc(icetSparseImageBufferSize(sub_size, 1));
    compress_sub = icetSparseImageAssignBuffer(compress_sub_buffer,
                                               sub_size, 1);

    sparse_copy_buffer = malloc(icetSparseImageBufferSize(sub_size, 1));
    sparse_copy = icetSparseImageAssignBuffer(sparse_copy_buffer,
                                              sub_size, 1);

    icetCompressSubImage(image, start, sub_size, compress_sub);

    icetCompressImage(image, full_sparse);
    icetSparseImageCopyPixels(full_sparse, start, sub_size, sparse_copy);

    result = CompareSparseImages(compress_sub, sparse_copy);

    free(full_sparse_buffer);
    free(compress_sub_buffer);
    free(sparse_copy_buffer);

    return result;
}

static int TestSparseImageCopyPixels(const IceTImage image)
{
#define NUM_OFFSETS 7
    IceTSizeType interesting_offset[NUM_OFFSETS];
    IceTSizeType width = icetImageGetWidth(image);
    IceTSizeType height = icetImageGetHeight(image);
    int start, end;

    if (height <= 20) {
        printf("Need image height greater than 20.\n");
        return TEST_NOT_RUN;
    }

    interesting_offset[0] = 0;  /* First pixel. */
    interesting_offset[1] = 10*width; /* Some pixels up at left. */
    interesting_offset[2] = 10*width + width/2; /* A bit up in the middle. */
    interesting_offset[3] = width*(height/2) + height/2; /* Middle at triangle diagonal. */
    interesting_offset[4] = width*(height-10); /* Some pixels from top at left. */
    interesting_offset[5] = width*(height-10) + width/2; /* Some pixels from top in middle. */
    interesting_offset[6] = width*height; /* Last pixel. */

    for (start = 0; start < NUM_OFFSETS; start++) {
        for (end = start+1; end < NUM_OFFSETS; end++) {
            int result;
            result = TrySparseImageCopyPixels(image,
                                              interesting_offset[start],
                                              interesting_offset[end]);
            if (result != TEST_PASSED) return result;
        }
    }

    return TEST_PASSED;
}

static int SparseImageCopyRun()
{
    IceTVoid *imagebuffer;
    IceTImage image;

    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);
    icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);

    imagebuffer = malloc(icetImageBufferSize(SCREEN_WIDTH, SCREEN_HEIGHT));
    image = icetImageAssignBuffer(imagebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);

    printf("\n********* Creating lower triangle image\n");
    LowerTriangleImage(image);

    if (TestSparseImageCopyPixels(image) != TEST_PASSED) {
        return TEST_FAILED;
    }

    printf("\n********* Creating upper triangle image\n");
    UpperTriangleImage(image);

    if (TestSparseImageCopyPixels(image) != TEST_PASSED) {
        return TEST_FAILED;
    }

    free(imagebuffer);

    return TEST_PASSED;
}

int SparseImageCopy(int argc, char *argv[])
{
    /* To remove warning */
    (void)argc;
    (void)argv;

    return run_test(SparseImageCopyRun);
}
