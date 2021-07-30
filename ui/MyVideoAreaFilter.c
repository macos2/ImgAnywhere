/*
 * MyVideoAreaSrc.c
 *
 *  Created on: 2021年7月16日
 *      Author: tom
 */
#include <gst/gst.h>
#include "MyVideoAreaFilter.h"
#include <gst/video/video.h>
#include "MyAreaFilterPad.h"

#define PRIV 	MyVideoAreaFilterPrivate *priv=my_video_area_filter_get_instance_private(self)

GST_DEBUG_CATEGORY_STATIC (my_video_area_filter_debug);
#define GST_CAT_DEFAULT my_video_area_filter_debug

typedef struct {
	guint *src_index;
} MyVideoAreaFilterPrivate;

G_DEFINE_TYPE_WITH_CODE(MyVideoAreaFilter, my_video_area_filter,
		GST_TYPE_ELEMENT, G_ADD_PRIVATE(MyVideoAreaFilter));

//GST_ELEMENT_REGISTER_DEFINE(video_area_filter,"videoareafilter",GST_RANK_NONE,MY_TYPE_VIDEO_AREA_FILTER);

//static GstStaticPadTemplate sink = { "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
//GST_STATIC_CAPS ("video/x-raw,format=(string){RGBx, ARGB}"), };
//
//static GstStaticPadTemplate src = { "src_%u", GST_PAD_SRC, GST_PAD_REQUEST,
//GST_STATIC_CAPS ("video/x-raw,format=(string){RGBx, ARGB}"), };

static GstStaticPadTemplate sink = { "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
GST_STATIC_CAPS ("video/x-raw"), };

static GstStaticPadTemplate src = { "src_%u", GST_PAD_SRC, GST_PAD_REQUEST,
GST_STATIC_CAPS ("video/x-raw"), };

GstFlowReturn my_video_area_filter_sink_chain(GstPad *sink_pad,
		MyVideoAreaFilter *self, GstBuffer *buf);
static gboolean my_video_area_filter_sink_event(GstPad *sink_pad,
		MyVideoAreaFilter *self, GstEvent *event);
GstFlowReturn my_video_area_filter_get_range(GstPad *src_pad,MyVideoAreaFilter *self,guint64 offset,
                           guint length,
                           GstBuffer **buffer);
static gboolean my_video_area_filter_src_event(GstPad *src_pad,
		MyVideoAreaFilter *self, GstEvent *event);
static gboolean my_video_area_filter_query(GstPad *pad, MyVideoAreaFilter *self,
		GstQuery *query);

GstPad* my_video_area_filter_request_pad(MyVideoAreaFilter *self,
		GstPadTemplate *templ, const gchar *name_templ, const GstCaps *caps) {
	PRIV;
	guint index;
	GstPad *srcpad;
	if (g_hash_table_contains(self->src_pad, name_templ)) {
		g_printerr("%s has been used used\n", name_templ);
		return NULL;
	}
	srcpad =
			GST_PAD_CAST(
					g_object_new (MY_TYPE_AREA_FILTER_PAD,"name", name_templ, "direction", templ->direction, "template", templ,NULL));
	MY_AREA_FILTER_PAD(srcpad)->index = index;
	gst_pad_set_query_function(srcpad, GST_DEBUG_FUNCPTR (my_video_area_filter_query));
	gst_pad_set_event_function(srcpad,GST_DEBUG_FUNCPTR (my_video_area_filter_src_event));
	gst_pad_set_getrange_function(srcpad,GST_DEBUG_FUNCPTR(my_video_area_filter_get_range));
	g_hash_table_insert(self->src_pad, name_templ, srcpad);
	gst_element_add_pad(GST_ELEMENT_CAST(self), srcpad);
	gboolean b=gst_pad_set_active(srcpad,TRUE);
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
	case GST_VIDEO_FORMAT_BGRA:
		isurf = cairo_image_surface_create_for_data(info.data,
				CAIRO_FORMAT_ARGB32, meta->width, meta->height,
				4 * meta->width);
		break;
	case GST_VIDEO_FORMAT_xBGR:
		isurf = cairo_image_surface_create_for_data(info.data,
				CAIRO_FORMAT_RGB24, meta->width, meta->height, 4 * meta->width);
		break;
	default:
		return GST_FLOW_ERROR;
		break;
	}

	//the output value
	GstBuffer *out;
	GstMemory *omem;
	MyAreaFilterPad *pad;
	cairo_t *cr;
	GList *l, *pads = g_hash_table_get_values(self->src_pad);
	l = pads;
	while (l != NULL) {
		pad = l->data;
		if (pad->area == NULL) {
			l = l->next;
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
		omem = gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
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
	GstEvent *e;
	GList *l, *pads = g_hash_table_get_values(self->src_pad);
	switch (event->type) {
	default:
		ret = gst_pad_event_default(sink_pad, self, event);
		break;
	}
	g_list_free(pads);
	return ret;
}

GstFlowReturn my_video_area_filter_get_range(GstPad *src_pad,MyVideoAreaFilter *self,guint64 offset,
                           guint length,
                           GstBuffer **buffer){

return gst_pad_pull_range(self->sink_pad,offset,length,buffer);
}

static gboolean my_video_area_filter_src_event(GstPad *src_pad,
		MyVideoAreaFilter *self, GstEvent *event) {
	gboolean ret = FALSE;
	GstEvent *e;
	GList *l, *pads = g_hash_table_get_values(self->src_pad);
	switch (event->type) {
	default:
		ret = gst_pad_event_default(src_pad, self, event);
		break;
	}
	g_list_free(pads);
	return ret;
}


static gboolean my_video_area_filter_query(GstPad *pad, MyVideoAreaFilter *self,
		GstQuery *query) {
	gboolean ret = FALSE;
	MyAreaFilterPad *p = pad == self->sink_pad ? NULL : pad;
	GstCaps *caps;
	gchar *t;
	gint w, h;
	GList *l;
	gint64 duration;
	g_print("Query:%s\n",p==NULL?"Sink":"Src");
	g_print("Type:%s\n",gst_query_type_get_name(query->type));
	switch (query->type) {
	case GST_QUERY_CAPS:
		if (p != NULL) {//src pad query
			w = p->area->w / 1;
			h = p->area->h / 1;
//			caps = gst_caps_new_simple("video/x-raw", "format",
//						G_TYPE_STRING, "BGRA", NULL);//"width", G_TYPE_INT, w, "height",
////						G_TYPE_INT, h,
////						NULL);
			caps=gst_caps_new_empty_simple("video/x-raw");
			gst_query_set_caps_result(query, caps);
			ret = TRUE;
		} else {//sink pad query
//			caps = gst_caps_new_simple("video/x-raw","format", G_TYPE_STRING, "BGRA",NULL);
			caps=gst_caps_new_empty_simple("video/x-raw");
			gst_query_set_caps_result(query, caps);
			ret = TRUE;
		}
		break;
	case GST_QUERY_DURATION:
		ret=gst_pad_peer_query_duration(self->sink_pad,GST_FORMAT_TIME,&duration);
		gst_query_set_duration(query,GST_FORMAT_TIME,duration);
	default:
		ret = gst_pad_query_default(pad, self, query);
		break;
	}
	g_print("return:%s\n\n",ret?"true":"false");
	return ret;
}

static void my_video_area_filter_class_init(MyVideoAreaFilterClass *klass) {
	GstElementClass *gstclass = klass;
	gst_element_class_set_static_metadata(gstclass, "area filiter", "my/filter",
			"return the special area content according to box area",
			"macos2@github.com");
	GST_DEBUG_CATEGORY_INIT(my_video_area_filter_debug,"MyVideoAreaFilter",0,"videoareafilter");
	gst_element_class_add_static_pad_template(gstclass, &sink);
	gst_element_class_add_static_pad_template(gstclass, &src);
	gstclass->release_pad = my_video_area_filter_release_pad;
	gstclass->request_new_pad = my_video_area_filter_request_pad;
}

static void my_video_area_filter_init(MyVideoAreaFilter *self) {
	self->sink_pad = gst_pad_new_from_static_template(&sink, "sink");
	gst_element_add_pad(self, self->sink_pad);
	gst_pad_set_event_function(self->sink_pad, GST_DEBUG_FUNCPTR (my_video_area_filter_sink_event));
	gst_pad_set_chain_function(self->sink_pad, GST_DEBUG_FUNCPTR (my_video_area_filter_sink_chain));
	gst_pad_set_query_function(self->sink_pad, GST_DEBUG_FUNCPTR (my_video_area_filter_query));
	self->src_pad = g_hash_table_new(g_str_hash, g_str_equal);
	gst_pad_set_active(self->sink_pad,TRUE);
}

#define PACKAGE "my-package"
//#define GST_PACKAGE_RELEASE_DATETIME "2021-07-20"
static gboolean plugin_init(GstPlugin *plugin) {
	return gst_element_register(plugin, "videoareafilter", GST_RANK_NONE,
	MY_TYPE_VIDEO_AREA);
}

GST_PLUGIN_DEFINE(0, 0, my, "filter video with special properity", plugin_init,
		"0", "LGPL", "MyPackage", "https://github.com/macos2")
