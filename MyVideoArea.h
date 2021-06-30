/*
 * MyVideoArea.h
 *
 *  Created on: 2021年6月10日
 *      Author: tom
 */

#ifndef MYVIDEOAREA_H_
#define MYVIDEOAREA_H_
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "ImgTool.h"
#define MY_TYPE_VIDEO_AREA my_video_area_get_type()

typedef enum{
	FIX_LEFT_TOP,
	FIX_LEFT_BOTTOM,
	FIX_RIGHT_TOP,
	FIX_RIGHT_BOTTOM,
	FIX_CENTRE,
	FIX_NONE,
}FixPoint;

typedef struct{
	gfloat w,h;
	gfloat offsetX,offsetY;
	cairo_matrix_t obj_mat;
}VideoBoxArea;

G_DECLARE_DERIVABLE_TYPE(MyVideoArea,my_video_area,MY,VIDEO_AREA,GtkDrawingArea);
typedef struct _MyVideoAreaClass{
	GtkDrawingAreaClass parent_class;
	void (*area_rotate)(VideoBoxArea *area,gdouble *angle);
	void (*area_move)(VideoBoxArea *area,gdouble *x,gdouble *y);
	void (*area_resize)(VideoBoxArea *area,gdouble *add_w,gdouble *add_h);
};

MyVideoArea *my_video_area_new();
void my_video_area_add_area(MyVideoArea *self,gchar *name,gfloat x,gfloat y,gfloat w,gfloat h);
void my_video_area_remove_area(MyVideoArea *self,gchar *name);
VideoBoxArea *my_video_area_get_area(MyVideoArea *self,gchar *name);
gchar *my_video_area_get_name(MyVideoArea *self,VideoBoxArea *area);
void my_video_area_move_area_by_name(MyVideoArea *self,gchar *name,gdouble x,gdouble y);
void my_video_area_rotate_area_by_name(MyVideoArea *self,gchar *name,FixPoint point,gfloat angle_degree);
void my_video_area_set_pixbuf(MyVideoArea *self,GdkPixbuf *pixbuf);
const GdkPixbuf *my_video_area_get_pixbuf(MyVideoArea *self);
void my_video_area_set_scale(MyVideoArea *self,gdouble scale);
cairo_surface_t *my_video_area_get_area_content_by_name(MyVideoArea *self,gchar *name);
#endif /* MYVIDEOAREA_H_ */
