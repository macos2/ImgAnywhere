/*
 * SurfFormat.c
 *
 *  Created on: 2021年6月28日
 *      Author: TOM
 */

#include "SurfFormat.h"

uint8_t bit_reversed(uint8_t n) {
	uint8_t t = 0;
	t = n & 0x01 << 7 + n & 0x02 << 5 + n & 0x04 << 3 + n & 0x08 << 1 + n
			& 0x10 >> 1 + n & 0x20 >> 3 + n & 0x40 >> 5 + n & 0x80 >> 7;
	return t;
}

gboolean need_align_bit_dir(ScanDirection first, BitDirection bdir) {
	if ((first == SCAN_DIR_LEFT_TO_RIGHT || first == SCAN_DIR_RIGHT_TO_LEFT)
			&& bdir == BIT_DIR_VERTICAL)
		return TRUE;
	if ((first == SCAN_DIR_BOTTOM_TO_TOP || first == SCAN_DIR_TOP_TO_BOTTOM)
			&& bdir == BIT_DIR_PARALLEL)
		return TRUE;
	return FALSE;
}

uint64_t surf_a1_transform_by_scan(cairo_surface_t *s, uint8_t **res,
		ScanDirection first, ScanDirection second, ByteOrder order,
		BitDirection bdir) {
	cairo_surface_t *temp = NULL;
	guint32 w, wb, h, i, j;
	guint8 *in, *out;
	cairo_format_t format = cairo_image_surface_get_format(s);
	cairo_t *cr = NULL;
	cairo_matrix_t Xmirror, Ymirror, Total, Origin;
	uint8_t *result = NULL;
	uint64_t length = 0;
	uint32_t *data;
	cairo_surface_mark_dirty(s); //make all the surface modify vaild or it will lost after the transform.
	if (need_align_bit_dir(first, bdir)) {
		//字节位的排列顺序为垂直方向。
		w = cairo_image_surface_get_height(s);
		h = cairo_image_surface_get_width(s);
		wb = (w + 7) / 8;
		result = malloc(h * wb);
		temp = cairo_image_surface_create(format, w, h);
		cr = cairo_create(temp);
		cairo_save(cr);
		//compute the matrix ,the image have been rotated 90 degree.
		if (first == SCAN_DIR_BOTTOM_TO_TOP
				|| second == SCAN_DIR_BOTTOM_TO_TOP) {
			cairo_matrix_init(&Xmirror, 1., 0, 0, 1., 0, 0);		//左右不变
		} else {		//SCAN_DIR_TOP_TO_BOTTOM
			cairo_matrix_init(&Xmirror, -1., 0, 0, 1., w, 0);	//左右翻转

		}
		if (first == SCAN_DIR_LEFT_TO_RIGHT
				|| second == SCAN_DIR_LEFT_TO_RIGHT) {
			cairo_matrix_init(&Ymirror, 1., 0, 0, 1., 0, 0);		//上下不变
		} else {		//SCAN_DIR_RIGHT_TO_LEFT
			cairo_matrix_init(&Ymirror, 1., 0, 0, -1., 0, h);	//上下翻转
		}
		cairo_matrix_init_translate(&Origin, 1. * w, 0);
		cairo_matrix_rotate(&Origin, 1. * G_PI_2);
		cairo_matrix_multiply(&Total, &Xmirror, &Ymirror);
		//Xmirror used for store the matrix compute result.
		cairo_matrix_multiply(&Xmirror, &Origin, &Total);
		cairo_set_matrix(cr, &Xmirror);
		cairo_set_source_surface(cr, s, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
		cairo_surface_flush(temp);
		cairo_destroy(cr);
		in = cairo_image_surface_get_data(temp);
		//Scan DATA
		out = result;
		if (first == SCAN_DIR_BOTTOM_TO_TOP
				|| first == SCAN_DIR_TOP_TO_BOTTOM) {
			for (i = 0; i < h; i++) {
				memcpy(out, in, wb);
				length += wb;
				out += wb;
				in += (w + 31) / 32 * 4;
			}
		} else {
			//first == SCAN_DIR_LEFT_TO_RIGHT || first == SCAN_DIR_RIGHT_TO_LEFT
			for (i = 0; i < wb; i++) {
				for (j = 0; j < h; j++) {
					*out = *(in + j * wb + i);
					length++;
					out++;
				}
			}
		}

	} else {
		//字节位的排列顺序为水平方向。
		h = cairo_image_surface_get_height(s);
		w = cairo_image_surface_get_width(s);
		wb = (w + 7) / 8;
		result = malloc(h * wb);
		temp = cairo_image_surface_create(format, w, h);
		cr = cairo_create(temp);
		//compute the matrix ,the image have not been rotated 90 degree.
		if (first == SCAN_DIR_LEFT_TO_RIGHT
				|| second == SCAN_DIR_LEFT_TO_RIGHT) {
			cairo_matrix_init(&Xmirror, 1., 0, 0, 1., 0, 0);	//左右不变
		} else {	//SCAN_DIR_RIGHT_TO_LEFT
			cairo_matrix_init(&Xmirror, -1., 0, 0, 1., w, 0);	//左右翻转
		}
		if (first == SCAN_DIR_BOTTOM_TO_TOP
				|| second == SCAN_DIR_BOTTOM_TO_TOP) {
			cairo_matrix_init(&Ymirror, 1., 0, 0, -1., 0, h);	//上下翻转
		} else {	//SCAN_DIR_TOP_TO_BOTTOM
			cairo_matrix_init(&Ymirror, 1., 0, 0, 1., 0, 0);	//上下不变
		}
		cairo_matrix_multiply(&Total, &Xmirror, &Ymirror);
		cairo_save(cr);
		cairo_set_matrix(cr, &Total);
		cairo_set_source_surface(cr, s, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
		cairo_surface_flush(temp);
		cairo_destroy(cr);
		in = cairo_image_surface_get_data(temp);
		//Scan DATA
		out = result;
		if (first == SCAN_DIR_LEFT_TO_RIGHT
				|| first == SCAN_DIR_RIGHT_TO_LEFT) {
			for (i = 0; i < h; i++) {
				memcpy(out, in, wb);
				out += wb;
				length += wb;
				in += (w + 31) / 32 * 4;
			}
		} else {
			//first==SCAN_DIR_BOTTOM_TO_TOP||first==SCAN_DIR_TOP_TO_BOTTOM
			for (i = 0; i < wb; i++) {
				for (j = 0; j < h; j++) {
					*out = *(in + j * wb + i);
					out++;
					length++;
				}
			}
		}
	}
	//cairo_surface_write_to_png(temp, "surfformat.png");
	cairo_surface_destroy(temp);
	if (order) {
		//Byte_MSB_FIRST special
		for (i = 0; i < length; i++) {
			result[i] = bit_reversed(result[i]);
		}
	}
	*res = result;
	return length;
}

cairo_surface_t* surf_a1_to_rgba_surf(cairo_surface_t *a1_surf,
		guint32 foreground, guint32 background) {
	if (a1_surf == NULL)
		return NULL;
	cairo_surface_mark_dirty(a1_surf);
	cairo_surface_t *argb;
	guint32 w, h, i, j;
	guint8 bit;
	guint32  *d,*s,b;
	w = cairo_image_surface_get_width(a1_surf);
	h = cairo_image_surface_get_height(a1_surf);
	argb = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	s = cairo_image_surface_get_data(a1_surf);
	d = cairo_image_surface_get_data(argb);
	for (i = 0; i < h; i++) {
		b = *s;
		bit = 0;
		for (j = 0; j < w; j++) {
			if (b & 0x01) {
				*d = foreground;
			} else {
				*d = background;
			}
			d++;
			bit++;
			b = b >> 1;
			if (bit > 31) {
				s++;
				b = *s;
				bit = 0;
			}
		}
		if(w%32)s++;//图像宽度非32bit对齐
	}
	cairo_surface_mark_dirty(argb);
	return argb;
}

cairo_surface_t* surf_a8_to_rgba_surf(cairo_surface_t *a8_surf,
	guint32 foreground, guint32 background) {
	if (a8_surf == NULL)
		return NULL;
	cairo_surface_mark_dirty(a8_surf);
	cairo_surface_t *argb;
	guint32 w, h, i, j;
	guint8 *s,*f,*b;
	guint8 *d;
	guint16 tmp;
	gdouble alpha;
	w = cairo_image_surface_get_width(a8_surf);
	h = cairo_image_surface_get_height(a8_surf);
	argb = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	s = cairo_image_surface_get_data(a8_surf);
	d = cairo_image_surface_get_data(argb);
	f=&foreground;
	b=&background;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
				alpha=*s/255.;
				//b channel
				tmp=alpha*f[0]+(1.-alpha)*b[0];
				tmp=tmp>0xff?0xff:tmp;
				d[0]=tmp;
				//g channel
				tmp=alpha*f[1]+(1.-alpha)*b[1];
				tmp=tmp>0xff?0xff:tmp;
				d[1]=tmp;
				//r channel
				tmp=alpha*f[2]+(1.-alpha)*b[2];
				tmp=tmp>0xff?0xff:tmp;
				d[2]=tmp;
				//a channel
				tmp=alpha*f[3]+(1.-alpha)*b[3];
				tmp=tmp>0xff?0xff:tmp;
				d[3]=tmp;
				//next pixel
				d+=4;
				s++;
			}
	}
	cairo_surface_mark_dirty(argb);
	return argb;
}

void surf_rgba_to_gray_color(cairo_surface_t *surf, MeanOpt mean) {
if (surf == NULL)
	return;
guint16 tmp;
gdouble x;
guint8 *p;
guint32 w, h, i, j;
cairo_surface_mark_dirty(surf);
w = cairo_image_surface_get_width(surf);
h = cairo_image_surface_get_height(surf);
p = cairo_image_surface_get_data(surf);
i = w * h;
for (j = 0; j < i; j++) {
	switch (mean) {
	case MEAN_NUM:
		tmp = (p[0] + p[1] + p[2]) / 3;
		break;
	case MEAN_GEO:
	default:
		x = p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
		tmp = ceil(sqrt(x) / 441.67295593 * 255.);
		//printf("%d %d %d = %d\n",p[1],p[2],p[3],tmp);
		break;
	}
	tmp = tmp > 0xff ? 0xff : tmp;
	p[0] = tmp;
	p[1] = tmp;
	p[2] = tmp;
	p += 4;
}
cairo_surface_mark_dirty(surf);
}

void surf_rgba_to_bw_color(cairo_surface_t *surf, MeanOpt mean,
	uint8_t threshold) {
if (surf == NULL)
	return;
guint16 tmp;
gdouble x;
guint8 *p;
guint32 w, h, i, j;
cairo_surface_mark_dirty(surf);
w = cairo_image_surface_get_width(surf);
h = cairo_image_surface_get_height(surf);
p = cairo_image_surface_get_data(surf);
i = w * h;
for (j = 0; j < i; j++) {
	switch (mean) {
	case MEAN_NUM:
		tmp = (p[0] + p[1] + p[2]) / 3;
		break;
	case MEAN_GEO:
	default:
		x = p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
		tmp = ceil(sqrt(x) / 441.67295593 * 255.);
		break;
	}
	tmp = tmp > threshold ? 0xff : 0x00;
	p[0] = tmp;
	p[1] = tmp;
	p[2] = tmp;
	p += 4;
}
cairo_surface_mark_dirty(surf);
}
