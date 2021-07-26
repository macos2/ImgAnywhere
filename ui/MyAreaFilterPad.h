/*
 * MyAreaFilterPad.h
 *
 *  Created on: 2021年7月19日
 *      Author: tom
 */

#ifndef UI_MYAREAFILTERPAD_H_
#define UI_MYAREAFILTERPAD_H_

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <gtk/gtk.h>
#include "MyVideoArea.h"
G_BEGIN_DECLS

#define MY_TYPE_AREA_FILTER_PAD my_area_filter_pad_get_type()

G_DECLARE_FINAL_TYPE(MyAreaFilterPad,my_area_filter_pad,MY,AREA_FILTER_PAD,GstPad);

//typedef struct _MyAreaFilterPadClass{
//GstPadClass parent_class;
//};

typedef struct _MyAreaFilterPad{
GstPad pad;
guint index;
VideoBoxArea *area;
};

MyAreaFilterPad *my_area_filter_pad_new();

G_END_DECLS


#endif /* UI_MYAREAFILTERPAD_H_ */
