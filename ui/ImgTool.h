/*
 * ImgTool.h
 *
 *  Created on: 2021年6月18日
 *      Author: tom
 */

#ifndef UI_IMGTOOL_H_
#define UI_IMGTOOL_H_

#include <math.h>
#include <stdint.h>

typedef enum{
	MEAN_NUM,
	MEAN_GEO,
}MeanOpt;

typedef struct{
	uint8_t top_left,		top_middle,		top_right;
	uint8_t left,			middle,			right;
	uint8_t bottom_left,	bottom_middle,	bottom_right;
}DiffRatio;

typedef struct{
	float RR,RG,RB,RA,RC;
	float GR,GG,GB,GA,GC;
	float BR,BG,BB,BA,BC;
	float AR,AG,AB,AA,AC;
}RemapWeight;

//some common static value
static DiffRatio diff_332={
		.right=3,.bottom_middle=3,.bottom_right=2,
};

static DiffRatio diff_332UP={
	.top_left=2,		.top_middle=3,		.top_right=2,
	.left=3,								.right=24,
	.bottom_left=12,	.bottom_middle=24,	.bottom_right=12,
};

static RemapWeight clear_alpha={
		.RR=1,.GG=1,.BB=1,.AA=0,.AC=0xff,
};
static RemapWeight r_channel={
		.RR=1,.AA=0,.AC=0xff,
};
static RemapWeight g_channel={
		.GG=1,.AA=0,.AC=0xff,
};
static RemapWeight b_channel={
		.BB=1,.AA=0,.AC=0xff,
};
static RemapWeight a_channel={
		.RR=0,.GG=0,.BB=0,.AA=1,
		.RA=1,.GA=1,.BA=1,
};

//remap the color space into gray with a channel
static RemapWeight a_as_gray={
		.RR=0,.GG=0,.BB=0,.AA=0,
		.RA=1,.GA=1,.BA=1,.AC=0xff,
};

//remap the color space into Black and White with a channel
static RemapWeight a_as_bw={
		.RR=0,.GG=0,.BB=0,.AA=0,
		.RA=0xff,.GA=0xff,.BA=0xff,.AC=0xff,
};

static uint8_t GrayMapWeight_n[]={1,1,2,2,2,4,4,4,4,4,4,1,1};
static uint8_t GrayMapWeight_index[]={0,1,2,4,6,8,12,16,20,24,28,32,33};
static uint8_t GrayMapWeight[]={
    1,1,1,
    1,1,1,//1
    1,1,1,

    1,1,1,
    1,0,1,//1
    1,1,1,

    1,1,1,
    0,0,0,//2
    1,1,1,

    1,0,1,
    1,0,1,
    1,0,1,

    1,0,1,
    0,1,0,//2
    1,0,1,

    0,1,0,
    1,1,1,
    0,1,0,

    1,0,1,
    0,0,0,//2
    1,0,1,

    0,1,0,
    1,0,1,
    0,1,0,

    1,0,0,
    0,1,1,//4
    1,0,0,

    0,0,1,
    1,1,0,
    0,0,1,

    0,1,0,
    0,1,0,
    1,0,1,

    1,0,1,
    0,1,0,
    0,1,0,

    1,0,0,
    1,1,0,//4
    1,0,0,

    0,0,1,
    0,1,1,
    0,0,1,

    1,1,1,
    0,1,0,
    0,0,0,

    0,0,0,
    0,1,0,
    1,1,1,

    1,0,0,
    0,0,1,//4
    1,0,0,

    0,0,1,
    1,0,0,
    0,0,1,

    1,0,1,
    0,0,0,
    0,1,0,

    0,1,0,
    0,0,0,
    1,0,1,

    1,0,0,
    0,1,0,//4
    1,0,0,

    0,0,1,
    0,1,0,
    0,0,1,

    1,0,1,
    0,1,0,
    0,0,0,

    0,0,0,
    0,1,0,
    1,0,1,

    0,1,0,
    0,1,0,//4
    0,1,0,

    0,0,0,
    1,1,1,
    0,0,0,

    1,0,0,
    0,1,0,
    0,0,1,

    0,0,1,
    0,1,0,
    1,0,0,

    0,1,0,
    0,0,0,//4
    0,1,0,

    0,0,0,
    1,0,1,
    0,0,0,

    0,0,1,
    0,0,0,
    1,0,0,

    1,0,0,
    0,0,0,
    0,0,1,

    0,0,0,
    0,1,0,//1
    0,0,0,

    0,0,0,
    0,0,0,//1
    0,0,0,
};



/*
 * ARGB 32bit image to GRAY 8bit image
 * @argb_img : pointer to 32bit ARGB image,32bit format A[31:24] R[23:16] G[15:8] B[7:0]
 * @gray_img : pointer to 8bit GRAY image
 * @w : image width
 * @h : image height
 * @mean : mean method 	MEAN_NUM or MEAN_GEO
 */
void img_argb_to_gray(uint8_t *argb_img, uint8_t *gray_img,
		uint32_t w,uint32_t h, MeanOpt mean);


/*
 * ARGB 32bit image each pixel remap to new ARGB 32bit image pixel with following formal
 * A=AR*R+AG*G+AB*B+AA*A+AC
 * R=RR*R+RG*G+RB*B+RA*A+RC
 * G=GR*R+GG*G+GB*B+GA*A+GC
 * B=BR*R+BG*G+BB*B+BA*A+BC
 */
void img_argb_remap(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,RemapWeight *weight);

/*
 * GRAY 8bit image to 1bit BLACK-WHITE image
 * @gray_img : pointer to 8bit GRAY image
 * @bit_img : pointer to 1bit image
 * @w : image width
 * @h : image height
 * @threshold : pixel GRAY value > threshold output WHITE (or 1)
 * 				pixel GRAY value < threshold output BLACK (or 0)
 */
void img_gray_to_bit(uint8_t *gray_img, uint32_t *bit_img,
		uint32_t w,uint32_t h, uint8_t threshold);

/*
 * inverse all the color byte value except the alpha value.
 * @argb : 32bit RGB or 32bit ARGB Format data.
 * @w : image width.
 * @h : image height.
 */
void img_argb_color_inv(uint8_t *argb,uint32_t w,uint32_t h);


/*
 * inverse all the byte value.
 * @img : pointer to the image data.
 * @w : image width.
 * @h : image height.
 * @pixel_size : each pixel size in byte.
 */
void img_byte_inv(uint8_t *img,uint32_t w,uint32_t h,uint8_t pixel_size);


/*
 * Compute Image pixel value in n-rank
 * @in : input image
 * @out : output image
 * @w_byte : image width in byte
 * @h : image height
 * @rank : number of rank
 */
void img_rank(uint8_t *in, uint8_t *out,
		uint32_t w_byte,uint32_t h, uint8_t rank);

/*
 * Process Image pixel value Error Diffusion
 * @in : input image
 * @out : output image
 * @w : image width
 * @h : image height
 * @pixel_size : a pixel size in byte.
 * @rank : number of rank
 * @tatio : the tatio of error to add to the pixels around the current pixel.
 */
void img_error_diffusion(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,uint8_t pixel_size,uint8_t rank,DiffRatio *tatio);

/*
 * Perform A Image Easy Edge Detect
 * @in : input image
 * @out : output image
 * @w : image width
 * @h : image height
 * @pixel_size : a pixel size in byte.
 * @rank : number of rank
 */
void img_edge_detect(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,uint8_t pixel_size);

void img_gray_map(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,uint8_t pixel_size,uint8_t seed);

#endif /* UI_IMGTOOL_H_ */
