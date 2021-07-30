/*
 * MyAreaFilterPad.c
 *
 *  Created on: 2021年7月19日
 *      Author: tom
 */


#include "MyAreaFilterPad.h"

enum {
	Prob_VideoBoxArea = 1,
	N_Prob,
}PadProb;


G_DEFINE_TYPE(MyAreaFilterPad,my_area_filter_pad,GST_TYPE_PAD);

void       my_area_filter_pad_set_property		(MyAreaFilterPad *self,
                                       guint           property_id,
                                       const GValue   *value,
                                       GParamSpec     *pspec){
	VideoBoxArea *area=NULL;
	switch (property_id){
	case Prob_VideoBoxArea:
		area=g_value_get_pointer(value);
		if(area!=NULL){
			GST_OBJECT_LOCK(self);
			self->area=area;
			GST_OBJECT_UNLOCK(self);
		}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self,property_id,pspec);
		break;
	}

}
void       my_area_filter_pad_get_property	(MyAreaFilterPad *self,
                                       guint           property_id,
                                       GValue         *value,
                                       GParamSpec     *pspec){
	switch (property_id){
	case Prob_VideoBoxArea:
		g_value_set_pointer(value,self->area);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self,property_id,pspec);
		break;
	}
}
void       my_area_filter_pad_dispose			(MyAreaFilterPad *self){

}
void       my_area_filter_pad_finalize		(MyAreaFilterPad *self){

}

static void my_area_filter_pad_class_init(MyAreaFilterPadClass *klass){
	GObjectClass *obj_class=klass;
	obj_class->set_property=my_area_filter_pad_set_property;
	obj_class->get_property=my_area_filter_pad_get_property;
	obj_class->dispose=my_area_filter_pad_dispose;
	obj_class->finalize=my_area_filter_pad_finalize;
	g_object_class_install_property(obj_class,Prob_VideoBoxArea,g_param_spec_pointer("boxarea","boxarea","the box area clip from pixbuf",G_PARAM_READWRITE));
}

static void my_area_filter_pad_init(MyAreaFilterPad *self){
self->area=NULL;
}

MyAreaFilterPad *my_area_filter_pad_new(){
return g_object_new(MY_TYPE_AREA_FILTER_PAD,NULL);
}
