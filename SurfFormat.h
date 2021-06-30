/*
 * SurfFormat
 *
 *  Created on: 2021年6月28日
 *      Author: TOM
 */

#ifndef SURFFORMAT_H_
#define SURFFORMAT_H_

#include <stdint.h>
#include <glib.h>
#include <cairo.h>
#include "ImgTool.h"

//扫描方向定义
typedef enum{
	SCAN_DIR_LEFT_TO_RIGHT,
	SCAN_DIR_RIGHT_TO_LEFT,
	SCAN_DIR_TOP_TO_BOTTOM,
	SCAN_DIR_BOTTOM_TO_TOP,
}ScanDirection;

//字节高低位次序
typedef enum{
	Byte_LSB_FIRST=0,
	Byte_MSB_FIRST,
}ByteOrder;

//字节位与扫描方向的关系。
typedef enum{
BIT_DIR_NONE,		//字节表示像素值，跟扫描方向无关，如灰阶、彩色图像。
BIT_DIR_PARALLEL,	//字节位代表一像素点，位排列方向跟扫描方向平行，如黑白图像，像素点只有黑(0)和白(1)两值。
BIT_DIR_VERTICAL,	//字节位代表一像素点，位排列方向跟扫描方向垂直，如黑白图像，像素点只有黑(0)和白(1)两值。
}BitDirection;

uint8_t bit_reversed(uint8_t n);


/*
 * a1surf_scan_tranform
 * convert the data according to the scan order.
 * @s : the cairo surface which is A1 format
 * @res : a pointer which would be modified point to the result.
 * @first & @second : the  first and second scan direction ,it should be across each other.or return invalid result.
 * @order : Byte_LSB_FIRST  or Byte_MSB_FIRST,or 0 for default Byte_LSB_FIRST.
 * @bdir : the relation between the bit of a byte array direction and the first scan direction.
 * @return : the length of the res data.
 */
uint64_t surf_a1_transform_by_scan(cairo_surface_t *s,uint8_t **res,ScanDirection first,ScanDirection second,ByteOrder order,BitDirection bdir);
cairo_surface_t *surf_a1_to_rgba_surf(cairo_surface_t *a1_surf,guint32 foreground,guint32 background);
cairo_surface_t *surf_a8_to_rgba_surf(cairo_surface_t *a8_surf,guint32 foreground,guint32 background);
void surf_rgba_to_gray_color(cairo_surface_t *surf,MeanOpt means);
void surf_rgba_to_bw_color(cairo_surface_t *surf,MeanOpt means,uint8_t threshold);



#endif /* SURFFORMAT_H_ */
