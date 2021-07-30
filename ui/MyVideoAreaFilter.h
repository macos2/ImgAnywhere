/*
 * MyVideoAreaSrc.h
 *
 *  Created on: 2021年7月16日
 *      Author: tom
 */

#ifndef UI_MYVIDEOAREAFILTER_H_
#define UI_MYVIDEOAREAFILTER_H_

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>
#include "MyVideoArea.h"

G_BEGIN_DECLS
#define MY_TYPE_VIDEO_AREA_FILTER (my_video_area_filter_get_type())
G_DECLARE_FINAL_TYPE(MyVideoAreaFilter,my_video_area_filter,MY,VIDEO_AREA_FILTER,GstElement);

typedef struct _MyVideoAreaFilter{
	GstElement element;
	GstPad *sink_pad;
	GHashTable *src_pad;
};

//GST_ELEMENT_REGISTER_DECLARE(my_video_area_filter);

G_END_DECLS


#endif /* UI_MYVIDEOAREAFILTER_H_ */
