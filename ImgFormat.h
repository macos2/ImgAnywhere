/*
 * Img2Format.h
 *
 *  Created on: 2021年6月21日
 *      Author: tom
 */

#ifndef IMGFORMAT_H_
#define IMGFORMAT_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef enum{
	RGB_FORMAT_444,		//0xRG,0xBR,0xGB,0xRG,0xBR...
	RGB_FORMAT_565,		//0b_RRRR RGGG , 0b_GGGB BBBB , 0b_RRRR RGGG , 0b_GGGB BBBB...
	RGB_FORMAT_666,		//0b_RRRR RR00 , 0b_GGGG GG00 , 0b_BBBB BB00 , 0b_RRRR RR00...
	RGB_FORMAT_888,		//0xRR,0xGG,0xBB,0xRR,0xGG,0xBB...
}RGBFormat;

/*
 * bit_map_format : a bit image to special binary format
 * @in : pointer to the bit image,image format follow the Cairo A1 format.
 * @out : a pointer address,the pointer to be modified to point to the output binary.
 * @w : image width
 * @h : image height
 * @step :  which the output width size should align to
 * return : the size of the output result.
 */
uint64_t bit_map_format (uint8_t *in, uint8_t **out, uint32_t w, uint32_t h,uint8_t step);

/*
 * rgb_format : the rgb or argb image to special binary format,be care the alpha channel is not include(or disable).
 * @in : pointer to the RGB image,image format follow the Cairo ARGB or RGB Format.
 * @out : a pointer address,the pointer to be modified to point to the output binary.
 * @w : image width.
 * @h : image height.
 * @format : output binary format.
 * return : the size of the output result.
 */
uint64_t rgb_format(uint32_t *rgb_in,uint8_t **out,uint32_t w,uint32_t h,RGBFormat format);

uint64_t data_to_string(uint8_t *data,uint64_t length,uint32_t col,char *row_start,char *fmt,char *seperate,char **res);
uint64_t data_to_c_array_string(uint8_t *data,uint64_t length,uint32_t col,char **res);
uint64_t data_to_asm_db_string(uint8_t *data,uint64_t length,uint32_t col,char **res);

#endif /* IMGFORMAT_H_ */
