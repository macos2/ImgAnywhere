/*
 * Img2Binary.h
 *
 *  Created on: 2021年6月21日
 *      Author: tom
 */

#ifndef IMG2BINARY_H_
#define IMG2BINARY_H_

#include <stdlib.h>
#include <stdint.h>

//扫描方向定义
typedef enum{
	SCAN_DIR_LEFT_TO_RIGHT,
	SCAN_DIR_RIGHT_TO_LEFT,
	SCAN_DIR_TOP_TO_BOTTOM,
	SCAN_DIR_BOTTOM_TO_TOP,
}ScanDirection;

//字节高低位次序
typedef enum{
	Byte_MSB_FIRST,
	Byte_LSB_FIRST,
}ByteOrder;

//字节位与扫描方向的关系。
typedef enum{
BIT_DIR_NONE,		//字节表示像素值，跟扫描方向无关，如灰阶、彩色图像。
BIT_DIR_PARALLEL,	//字节位代表一像素点，跟扫描方向平行，如黑白图像，像素点只有黑(0)和白(1)两值。
BIT_DIR_VERTICAL,	//字节位代表一像素点，跟扫描方向垂直，如黑白图像，像素点只有黑(0)和白(1)两值。
}BitDirection;

typedef enum{
	RGB444,//0xRG,0xBR,0xGB,0xRG,0xBR...
	RGB565,//0b_RRRR RGGG , 0b_GGGB BBBB , 0b_RRRR RGGG , 0b_GGGB BBBB...
	RGB666,//0b_RRRR RR00 , 0b_GGGG GG00 , 0b_BBBB BB00 , 0b_RRRR RR00...
}RGBFormat;

/*
 * bit_to_binary : a bit image to special binary format
 * @in : pointer to the bit image,image format follow the Cairo A1 format.
 * @out : a pointer address,the pointer to be modified to point to the output binary.
 * @w : image width
 * @h : image height
 * @first_dir,@second_dir:the first and second scan direction
 * @bit_dir : the relation between the bit and the scan direction
 * @bit_order : the bit order in byte.
 * @scan_w : the scan width in byte
 * return : the size of the output result.
 */
uint64_t bit_to_binary(uint8_t *in,uint8_t **out,uint32_t w,uint32_t h,ScanDirection first_dir,ScanDirection second_dir,BitDirection bit_dir,ByteOrder bit_order,uint8_t scan_w);

/*
 * rgb_to_binary : the rgb or argb image to special binary format,be care the alpha channel is not include(or disable).
 * @in : pointer to the RGB image,image format follow the Cairo ARGB or RGB format.
 * @out : a pointer address,the pointer to be modified to point to the output binary.
 * @w : image width.
 * @h : image height.
 * @format : output binary format.
 * return : the size of the output result.
 */
uint64_t rgb_to_binary(uint8_t *rgb_in,uint8_t **out,uint32_t w,uint32_t h,RGBFormat format);

#endif /* IMG2BINARY_H_ */
