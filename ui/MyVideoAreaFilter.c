/*
 * MyVideoAreaSrc.c
 *
 *  Created on: 2021年7月16日
 *      Author: tom
 */

#include "MyVideoAreaFilter.h"
#include <gst/video/video.h>
#include "MyAreaFilterPad.h"

#define PRIV 	MyVideoAreaFilterPrivate *priv=my_video_area_filter_get_instance_private(self)

typedef struct {
	guint *src_index;
} MyVideoAreaFilterPrivate;

G_DEFINE_TYPE_WITH_CODE(MyVideoAreaFilter, my_video_area_filter,
		GST_TYPE_ELEMENT, G_ADD_PRIVATE(MyVideoAreaFilter));

//GST_ELEMENT_REGISTER_DEFINE(my_video_area_filter,"video-area-filter",GST_RANK_NONE,MY_TYPE_VIDEO_AREA);

static GstStaticPadTemplate sink = { "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
GST_STATIC_CAPS ("video/x-raw,format=(string){xRGB,ARGB}"), };

static GstStaticPadTemplate src = { "src_%u", GST_PAD_SRC, GST_PAD_REQUEST,
GST_STATIC_CAPS ("video/x-raw,format=(string){xRGB,ARGB}"), };

GstFlowReturn my_video_area_filter_sink_chain(GstPad *sink_pad,
		MyVideoAreaFilter *self, GstBuffer *buf);
static gboolean my_video_area_filter_sink_event(GstPad *sink_pad,
		MyVideoAreaFilter *self, GstEvent *event);
static gboolean my_video_area_filter_src_query(MyAreaFilterPad *pad,
		MyVideoAreaFilter *self, GstQuery *query);

GstPad* my_video_area_filter_request_pad(MyVideoAreaFilter *self,
		GstPadTemplate *templ, const gchar *name_templ, const GstCaps *caps) {
	PRIV;
	guint index;
	GstPad *srcpad;
	gchar *pad_name;
	if (name_templ && sscanf(name_templ, "src_%u", &index) == 1) {
		//name_templ is special ,check if it is vaild.
		if (g_hash_table_contains(self->src_pad, GUINT_TO_POINTER(index))) {
			g_printerr("%s has been used used\n", name_templ);
			return NULL;
		}
		if (index >= priv->src_index)
			priv->src_index = index + 1;
		pad_name = g_strdup(name_templ);
	} else {
		//name_templ is not special, general it with new index.
		index = priv->src_index;
		priv->src_index++;
		pad_name = g_strdup_printf("src_%u", index);
	}
	srcpad =
			GST_PAD_CAST(
					g_object_new (MY_TYPE_AREA_FILTER_PAD,"name", pad_name, "direction", templ->direction, "template", templ,NULL));
	MY_AREA_FILTER_PAD(srcpad)->index = index;
	gst_pad_set_query_function(srcpad, my_video_area_filter_src_query);
	g_hash_table_insert(self->src_pad, pad_name, srcpad);
	gst_element_add_pad(GST_ELEMENT_CAST(self), srcpad);
	g_free(pad_name);
	return srcpad;
}

void my_video_area_filter_release_pad(MyVideoAreaFilter *self, GstPad *pad) {
	if (pad == self->sink_pad)
		return;
	gchar *name = gst_pad_get_name(pad);
	MyAreaFilterPad *p = pad;
	if (!g_hash_table_contains(self->src_pad, name))
		return;
	g_hash_table_remove(self->src_pad, name);
}

//the pad chain,event and query.
GstFlowReturn my_video_area_filter_sink_chain(GstPad *sink_pad,
		MyVideoAreaFilter *self, GstBuffer *buf) {
	GstVideoMeta *meta = gst_buffer_get_video_meta(buf);
	if (meta == NULL)
		return GST_FLOW_ERROR;
	GstMemory *mem = gst_buffer_get_all_memory(buf);
	GstMapInfo info;
	gst_memory_map(mem, &info, GST_MAP_READ);
	cairo_surface_t *isurf = NULL;
	switch (meta->format) {
	case GST_VIDEO_FORMAT_ARGB:
		isurf = cairo_image_surface_create_for_data(info.data,
				CAIRO_FORMAT_ARGB32, meta->width, meta->height,
				4 * meta->width);
		break;
	case GST_VIDEO_FORMAT_xRGB:
			isurf = cairo_image_surface_create_for_data(info.data,
					CAIRO_FORMAT_RGB24, meta->width, meta->height,
					4 * meta->width);
		break;
	default:
		return GST_FLOW_ERROR;
		break;
	}

	//the output value
	GstBuffer *out;
	MyAreaFilterPad *pad;
	cairo_t *cr;
	GList *l, *pads = g_hash_table_get_values(self->src_pad);
	l = pads;
	while (l != NULL) {
		pad = l->data;
		if(pad->area==NULL){
			l=l->next;
			continue;
		}
		//process the output data with the srcpad
		out = gst_buffer_new();
		cairo_surface_t *osurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				pad->area->w, pad->area->h);
		GstVideoMeta *ometa = gst_buffer_add_video_meta(out, meta->flags,
				meta->format, pad->area->w, pad->area->h);
		cr = cairo_create(osurf);
		cairo_save(cr);
		my_video_area_to_area_coordinate(cr, pad->area);
		cairo_set_source_surface(cr, isurf, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
		cairo_destroy(cr);
		GstMemory *omem = gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
				cairo_image_surface_get_data(osurf),
				cairo_image_surface_get_stride(osurf)
						* cairo_image_surface_get_height(osurf), 0,
				cairo_image_surface_get_stride(osurf)
						* cairo_image_surface_get_height(osurf), osurf,
				cairo_surface_destroy);
		gst_buffer_append_memory(out, omem);
		gst_pad_push(pad, out);
		l = l->next;
	}
	g_list_free(pads);
	cairo_surface_destroy(isurf);
	gst_memory_unref(mem);
	gst_buffer_unref(buf);
	return GST_FLOW_OK;
}
static gboolean my_video_area_filter_sink_event(GstPad *sink_pad,
		MyVideoAreaFilter *self, GstEvent *event) {
	gboolean ret = FALSE;
	GList *l, *pads = g_hash_table_get_values(self->src_pad);
	switch (event->type) {
	default:
		l = pads;
		while (l != NULL) {
			ret = gst_pad_event_default(l->data, self, event);
			l = l->next;
		}
		break;
	}
	g_list_free(pads);
	return ret;
}

static gboolean my_video_area_filter_src_query(MyAreaFilterPad *pad,
		MyVideoAreaFilter *self, GstQuery *query) {
	gboolean ret = FALSE;
	GstCaps *caps, *sink_caps;
	switch (query->type) {
	case GST_QUERY_CAPS:
		sink_caps = gst_pad_get_current_caps(self->sink_pad);
		if(sink_caps ==NULL)return ret;
		caps = gst_caps_copy(sink_caps);
		gst_caps_set_simple(caps, "width", G_TYPE_INT, pad->area->w / 1,
				"height",
				G_TYPE_INT, pad->area->h / 1, "format", G_TYPE_STRING, "ARGB",
				NULL);
		gst_query_set_caps_result(query, caps);
		ret = TRUE;
		break;
	default:
		ret = gst_pad_query_default(self->sink_pad, self, query);
		break;
	}
	return ret;
}

static void my_video_area_filter_class_init(MyVideoAreaFilterClass *klass) {
	GstElementClass *gstclass = klass;
	gst_element_class_set_static_metadata(gstclass, "area filiter", "my/filter",
			"return the special area content according to box area",
			"macos2@github.com");
	gst_element_class_add_static_pad_template(gstclass, &sink);
	gst_element_class_add_static_pad_template(gstclass, &src);
	gstclass->release_pad = my_video_area_filter_release_pad;
	gstclass->request_new_pad = my_video_area_filter_request_pad;
}

static void my_video_area_filter_init(MyVideoAreaFilter *self) {
	self->sink_pad = gst_pad_new_from_static_template(&sink, "sink");
	gst_element_add_pad(self, self->sink_pad);
	gst_pad_set_event_function(self->sink_pad, my_video_area_filter_sink_event);
	gst_pad_set_chain_function(self->sink_pad, my_video_area_filter_sink_chain);
	self->src_pad = g_hash_table_new(g_str_hash, g_str_equal);
}

#define PACKAGE "my-package"
//#define GST_PACKAGE_RELEASE_DATETIME "2021-07-20"
static gboolean plugin_init(GstPlugin *plugin) {
	if (!gst_element_register(plugin, "videoareafilter", GST_RANK_NONE,
	MY_TYPE_VIDEO_AREA))
		return FALSE;
	return TRUE;
}

GST_PLUGIN_DEFINE(0, 0, my, "filter video with special properity", plugin_init,
		"0", "LGPL", "MyPackage", "https://github.com/macos2")

