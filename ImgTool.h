/*
 * ImgTool.h
 *
 *  Created on: 2021年6月18日
 *      Author: tom
 */

#ifndef IMGTOOL_H_
#define IMGTOOL_H_

#include <math.h>
#include <stdint.h>

typedef enum{
	MEAN_NUM,
	MEAN_GEO,
}MeanOpt;

typedef struct{
	//uint8_t lt,tm,rt;	//left-top 		top-middle 		right-top
	//uint8_t r,l;		//left 			[current-pixel]	right
	//uint8_t lb,bm,rb;	//left-bottom	bottom-middle	right-bottom
	uint8_t r,bm,rb;
}DiffRatio;

typedef struct{
	uint8_t RR,RG,RB,RA,RC;
	uint8_t GR,GG,GB,GA,GC;
	uint8_t BR,BG,BB,BA,BC;
	uint8_t AR,AG,AB,AA,AC;
}RemapWeight;

//some common static value
static DiffRatio diff_332={
		.r=3,.bm=3,.rb=2,
};

static RemapWeight disable_alpha={
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

/*
 * ARGB 32bit image to GRAY 8bit image
 * @argb_img : pointer to 32bit ARGB image,32bit format A[31:24] R[23:16] G[15:8] B[7:0]
 * @gray_img : pointer to 8bit GRAY image
 * @w : image width
 * @h : image height
 * @mean : mean method 	MEAN_NUM or MEAN_GEO
 */
void argb_to_gray(uint8_t *argb_img, uint8_t *gray_img,
		uint32_t w,uint32_t h, MeanOpt mean);


/*
 * ARGB 32bit image each pixel remap to new ARGB 32bit image pixel with following formal
 * A=AR*R+AG*G+AB*B+AA*A+AC
 * R=RR*R+RG*G+RB*B+RA*A+RC
 * G=GR*R+GG*G+GB*B+GA*A+GC
 * B=BR*R+BG*G+BB*B+BA*A+BC
 */
void argb_remap(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,RemapWeight *weight);

/*
 * GRAY 8bit image to 1bit BLACK-WHITE image
 * @gray_img : pointer to 8bit GRAY image
 * @bit_img : pointer to 1bit image
 * @w : image width
 * @h : image height
 * @threshold : pixel GRAY value > threshold output WHITE (or 1)
 * 				pixel GRAY value < threshold output BLACK (or 0)
 */
void gray_to_bit(uint8_t *gray_img, uint32_t *bit_img,
		uint32_t w,uint32_t h, uint8_t threshold);

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

#endif /* IMGTOOL_H_ */
