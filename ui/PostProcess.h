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
#include <gtk/gtk.h>
#include "../ImgFormat.h"
#include "../ImgTool.h"
#include "../SurfFormat.h"

G_BEGIN_DECLS

typedef enum {
	POST_NONE,
	POST_GRAY,
	POST_RGB_FMT,
	POST_ARGB_REMAP,
	POST_DIFFUSE,
	POST_TRANSPARENT,
	POST_BITMAP,
	OUT_WINDOWS,
	OUT_FILE,
	OUT_IMG_FILE,
} PostType;

typedef struct {
	PostType post_type;
	guint32 w, h;
	gboolean can_display;
	gpointer next_post;
} PostCommon;

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
	uint8_t rank, pixal_size,radio;
} PostDiffuse;

typedef struct {
	PostCommon com;
	gboolean transparent;
	guint8 r, g, b, a;
	gdouble color_distance;
} PostTransparent;

typedef enum {
	GRAY_SIM_NONE, GRAY_SIM_MUL_THRESOLD, GRAY_SIM_DIFFUSE,
} GraySim;

typedef struct {
	PostCommon com;
	GraySim gray;
	guint8 thresold, gray_rank;
	ScanDirection first, second;
	ByteOrder order;
	BitDirection bitdir;
} PostBitmap;

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
	GIOStream *file;
} OutFile;

typedef struct {
	PostCommon com;
	gchar *name_fmt;
	gboolean over_write;
	guint32 index;
} OutImgFile;

typedef gboolean (*PostProcessFunc)(PostCommon *post,gpointer data, gpointer *out);
PostProcessFunc get_post_process_func(PostCommon *post);
void append_post_process(PostCommon *post,PostCommon *append_post);
PostCommon *next_post_process(PostCommon *post);

gboolean post_process(PostCommon *post,gpointer data,gpointer *out);
gboolean post_gray(PostGray *gray,gpointer data,gpointer *out);
gboolean post_rgb_fmt(PostRGBFmt *rgbfmt,gpointer data,gpointer *out);
gboolean post_argb_remap(PostARGBRemap *argbremap,gpointer data,gpointer *out);
gboolean post_diffuse(PostDiffuse *diffuse,gpointer data,gpointer *out);
gboolean post_transparent(PostTransparent *transparent,gpointer data,gpointer *out);
gboolean post_bitmap(PostBitmap *bitmap,gpointer data,gpointer *out);
gboolean out_windows(OutWindow *window,gpointer data,gpointer *out);
gboolean out_file(OutFile *file,gpointer data,gpointer *out);
gboolean out_img_file(OutImgFile *img,gpointer data,gpointer *out);
G_END_DECLS

#endif /* UI_POSTPROCESS_H_ */
