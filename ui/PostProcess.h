/*
 * PostPocessor.h
 *
 *  Created on: 2021年7月29日
 *      Author: tom
 */

#ifndef UI_POSTPROCESS_H_
#define UI_POSTPROCESS_H_

#include <glib.h>
#include <glib-object.h>
#include <cairo.h>
#include <gtk/gtk.h>

#include "ImgFormat.h"
#include "ImgTool.h"
#include "MyLogo.h"
#include "SurfFormat.h"

G_BEGIN_DECLS

typedef enum {
	POST_NONE,
	POST_BW,
	POST_GRAY,
	POST_RGB_FMT,
	POST_ARGB_REMAP,
	POST_DIFFUSE,
	POST_TRANSPARENT,
	POST_BITMAP,
	POST_RESIZE,
	POST_FRAMERATE,
	OUT_WINDOWS,
	OUT_FILE,
	OUT_IMG_FILE,
} PostType;

typedef struct{
	guint framerate_n;
	guint framerate_d;
	guint32 w, h;
	guint64 output_size;
	guint64 timestamp;
	gpointer output_data;
}PostTransferData;

typedef struct {
	guint32 area_id;
	gchar *name;
	PostType post_type;
	//link to the MyMainPrivate->widget_draw_queue to redraw the display widget.
	GAsyncQueue *widget_draw_queue;
	//link to the MyMainPrivate->framerate_n & framerate_d for default init.
	guint *framerate_n,*framerate_d;
	//link to the MyMainPrivate->position & MyMainPrivate->duration.
	guint64 *duration;
	//transfer the output data in the thread.
	PostTransferData *transferdata;
} PostCommon;

typedef struct {
	PostCommon com;
	MeanOpt mean;
	guint8 thresold;
} PostBw;

typedef struct {
	PostCommon com;
	MeanOpt mean;
} PostGray;

typedef struct {
	PostCommon com;
	RGBFormat fmt;
} PostRGBFmt;

typedef struct {
	PostCommon com;
	RemapWeight remap_weight;
} PostARGBRemap;

typedef struct {
	PostCommon com;
	uint8_t rank;
	DiffRatio radio;
} PostDiffuse;

typedef struct {
	PostCommon com;
	gboolean transparent;
	guint8 r, g, b, a;
	guint8 ir, ig, ib, ia;//invert color 反色
	gdouble color_distance;
} PostTransparent;

typedef enum {
	GRAY_SIM_NONE, GRAY_SIM_MUL_THRESOLD, GRAY_SIM_DIFFUSE,
} GraySim;

typedef struct {
	PostCommon com;
	GraySim gray;
	guint8 thresold, gray_rank,diff_radio;
	MeanOpt mean;
	ScanDirection first, second;
	ByteOrder order;
	BitDirection bitdir;
	guint8 rank_index;
	gint8 rank_dir;
} PostBitmap;

typedef struct{
	PostCommon com;
	guint resize_w,resize_h;
	gboolean expand;
	gboolean full;
}PostResize;

typedef struct{
	PostCommon com;
	guint n,d;
	guint64 interval;
	guint64 previous_pos;
}PostFramerate;

typedef struct {
	PostCommon com;
	GtkWidget *display_widget;
} OutWindow;

typedef struct {
	PostCommon com;
	gchar *filename;
	gboolean c_source;
	gboolean asm_source;
	gboolean head_output;
	gboolean over_write;
	GOutputStream *out;
	guint64 index;
} OutFile;

typedef struct {
	PostCommon com;
	gchar *name_fmt;
	gchar *directory;
	gboolean over_write;
	guint32 index;
} OutImgFile;

typedef gboolean (*PostProcessFunc)(PostCommon *post,guint8 **data, gpointer *out);
PostProcessFunc post_get_process_func(PostCommon *post);
gboolean post_process(PostCommon *post,guint8 **data,gpointer *out);
GdkPixbuf *post_preview(PostCommon *post,cairo_surface_t **s);
void post_run(PostCommon *post);
void post_stop(PostCommon *post);
void post_free(PostCommon *post);


gboolean post_bw(PostBw *bw,cairo_surface_t **s,gpointer *out);
gboolean post_gray(PostGray *gray,cairo_surface_t **s,gpointer *out);
gboolean post_rgb_fmt(PostRGBFmt *rgbfmt,cairo_surface_t **s,gpointer *out);
gboolean post_argb_remap(PostARGBRemap *argbremap,cairo_surface_t **s,gpointer *out);
gboolean post_diffuse(PostDiffuse *diffuse,cairo_surface_t **s,gpointer *out);
gboolean post_transparent(PostTransparent *transparent,cairo_surface_t **s,gpointer *out);
gboolean post_bitmap(PostBitmap *bitmap,cairo_surface_t **s,gpointer *out);
gboolean post_resize(PostResize *resize,cairo_surface_t **s,gpointer *out);
gboolean post_framerate(PostFramerate *framerate, cairo_surface_t **s, gpointer *out);
void create_display_widget(OutWindow *post);
gboolean out_windows(OutWindow *window,cairo_surface_t **s,gpointer *out);
gboolean out_file(OutFile *file,cairo_surface_t **s,gpointer *out);
gboolean out_img_file(OutImgFile *img,cairo_surface_t **s,gpointer *out);
G_END_DECLS

#endif /* UI_POSTPROCESS_H_ */
