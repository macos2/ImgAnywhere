/*
 * MyMain.c
 *
 *  Created on: 2021年7月1日
 *      Author: TOM
 */

#include "MyMain.h"

#define GET_PRIV   MyMainPrivate *priv=my_main_get_instance_private(self)

typedef struct {
	VideoBoxArea *area;
	GdkPixbuf *pixbuf;
	GtkListStore *process_list;
} AreaInfo;

typedef struct {
	GtkAdjustment *pos_x, *pos_y, *progress, *rotate, *size_h, *size_w, *volume;
	GtkScrolledWindow *video_box;
	MyVideoArea *video_area;
	GtkTreeView *modifier_view;
	GtkPaned *paned;
	AreaInfo *current_area;
	GtkToggleButton *size_radio_lock;
	GtkEntry *area_label;
	GHashTable *area_table;
	gdouble r_w_h;
} MyMainPrivate;

G_DEFINE_TYPE_WITH_CODE(MyMain, my_main, GTK_TYPE_WINDOW,
		G_ADD_PRIVATE(MyMain));

enum {
	col_id, col_name, col_pixbuf, col_type, col_data, num_col,
};

GtkListStore* create_process_list() {
	GtkListStore *list = gtk_list_store_new(num_col, G_TYPE_UINT, G_TYPE_STRING,
	GDK_TYPE_PIXBUF, G_TYPE_UINT,
	G_TYPE_POINTER);
	return list;
}

void update_area_info(VideoBoxArea *area, MyMain *self) {
	GET_PRIV;
	double ac, as, angle_offset;
	ac = area->obj_mat.xx;
	as = area->obj_mat.xy;
	if (as > 0 && ac > 0) {
		angle_offset = 0.;
		// angle < 90
	} else if (as > 0 && ac < 0) {
		angle_offset = 90.;
		// 180 > angle > 90
	} else if (as < 0 && ac < 0) {
		angle_offset = 180.;
		// 270 > angle >180
	} else { //as<0 && ac>0
		angle_offset = 270.;
		// angle > 270
	}
	//ac=ac>0?ac:ac*-1.;
	ac = acos(ac);
	ac = ac * 180. / M_PI;
	if (angle_offset < 180.) {
		gtk_adjustment_set_value(priv->rotate, ac);
	} else {
		gtk_adjustment_set_value(priv->rotate, 360. - ac);
	}
	gtk_adjustment_set_value(priv->pos_x, area->offsetX);
	gtk_adjustment_set_value(priv->pos_y, area->offsetY);
	gtk_adjustment_set_value(priv->size_w, area->w);
	gtk_adjustment_set_value(priv->size_h, area->h);
	gtk_entry_set_text(priv->area_label, area->label);

}

void area_select(MyVideoArea *video_area, VideoBoxArea *area, MyMain *self) {
	gboolean radio;
	GET_PRIV;
	priv->current_area = g_hash_table_lookup(priv->area_table, area);
	if(priv->current_area==NULL)return;
	radio=gtk_toggle_button_get_active(priv->size_radio_lock);
	gtk_toggle_button_set_active(priv->size_radio_lock,FALSE);
	update_area_info(area, self);
	priv->r_w_h = area->w / area->h;
	gtk_toggle_button_set_active(priv->size_radio_lock,radio);
	AreaInfo *info = g_hash_table_lookup(priv->area_table, area);
	gtk_tree_view_set_model(priv->modifier_view, info->process_list);
}

void area_move(MyVideoArea *video_area, VideoBoxArea *area, gdouble *x,
		gdouble *y, MyMain *self) {
	GET_PRIV;
	//update_area_info (area, self);
	gtk_adjustment_set_value(priv->pos_x, area->offsetX);
	gtk_adjustment_set_value(priv->pos_y, area->offsetY);
}

void area_resize(MyVideoArea *video_area, VideoBoxArea *area, gdouble *add_w,
		gdouble *add_h, MyMain *self) {
	GET_PRIV;
	//update_area_info (area, self);
	if (gtk_toggle_button_get_active(priv->size_radio_lock)) {
		gdouble r = priv->r_w_h;
		priv->current_area->area->w-=*add_w;
		priv->current_area->area->h-=*add_h;
		*add_w = r * *add_h;
		priv->current_area->area->w+=*add_w;
		priv->current_area->area->h+=*add_h;
	}
	gtk_adjustment_set_value(priv->size_w, area->w);
	gtk_adjustment_set_value(priv->size_h, area->h);
}

void area_rotate(MyVideoArea *video_area, VideoBoxArea *area, gdouble *angle,
		MyMain *self) {
	GET_PRIV;
	update_area_info(area, self);
	double ac, as, angle_offset;
	ac = area->obj_mat.xx;
	as = area->obj_mat.xy;
	if (as > 0 && ac > 0) {
		angle_offset = 0.;
		// angle < 90
	} else if (as > 0 && ac < 0) {
		angle_offset = 90.;
		// 180 > angle > 90
	} else if (as < 0 && ac < 0) {
		angle_offset = 180.;
		// 270 > angle >180
	} else { //as<0 && ac>0
		angle_offset = 270.;
		// angle > 270
	}
	//ac=ac>0?ac:ac*-1.;
	ac = acos(ac);
	ac = ac * 180. / M_PI;
	if (angle_offset < 180.) {
		angle_offset = ac;
	} else {
		angle_offset = 360. - ac;
	}
	gtk_adjustment_set_value(priv->rotate, angle_offset);
}

void label_changed_cb(GtkEntry *label, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	gchar *new_name = gtk_entry_get_text(label);
	my_video_area_rename_area(priv->video_area, priv->current_area->area->label,
			new_name);
	gtk_widget_queue_draw(priv->video_area);
}

void size_changed_cb(GtkSpinButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	gdouble value = gtk_spin_button_get_value(button);
	GtkAdjustment *adj = gtk_spin_button_get_adjustment(button);
	if (adj == priv->size_w) {
		priv->current_area->area->w = value;
		if (gtk_toggle_button_get_active(priv->size_radio_lock)
				&& priv->r_w_h != 0.) {
			priv->current_area->area->h = value / priv->r_w_h;
			gtk_adjustment_set_value(priv->size_h, value / priv->r_w_h);
		}
	}
	if (adj == priv->size_h) {
		priv->current_area->area->h = value;
		if (gtk_toggle_button_get_active(priv->size_radio_lock)
				&& priv->r_w_h != 0.) {
			priv->current_area->area->w = value * priv->r_w_h;
			gtk_adjustment_set_value(priv->size_w, value * priv->r_w_h);
		}
	}
	gtk_widget_queue_draw(priv->video_area);
}

void pos_changed_cb(GtkSpinButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	gdouble value = gtk_spin_button_get_value(button);
	GtkAdjustment *adj = gtk_spin_button_get_adjustment(button);
	if (adj == priv->pos_x)
		priv->current_area->area->offsetX = value;
	if (adj == priv->pos_y)
		priv->current_area->area->offsetY = value;
	gtk_widget_queue_draw(priv->video_area);
}

void rotate_changed_cb(GtkSpinButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	gdouble value = gtk_spin_button_get_value(button) * -1. * M_PI / 180.;
	cairo_matrix_t rotate;
	cairo_matrix_init_rotate(&rotate, value);
	priv->current_area->area->obj_mat.xx = rotate.xx;
	priv->current_area->area->obj_mat.xy = rotate.xy;
	priv->current_area->area->obj_mat.yy = rotate.yy;
	priv->current_area->area->obj_mat.yx = rotate.yx;
	gtk_widget_queue_draw(priv->video_area);
}

void scale_changed_cb(GtkSpinButton *button, MyMain *self) {
	GET_PRIV;
	gdouble h_max,w_max,old_scale;
	GtkAdjustment *h=gtk_scrolled_window_get_hadjustment(priv->video_box);
	GtkAdjustment *w=gtk_scrolled_window_get_vadjustment(priv->video_box);
	h_max=gtk_adjustment_get_upper(h);
	w_max=gtk_adjustment_get_upper(w);
	gdouble hr=gtk_adjustment_get_value(h);
	gdouble wr=gtk_adjustment_get_value(w);
	if(h_max!=0)
		hr/=h_max;
		//old_scale=h_max/gdk_pixbuf_get_height(my_video_area_get_pixbuf(priv->video_area));
	else
		hr=0.5;
	if(w_max!=0)
		wr/=w_max;
	else
		wr=0.5;
	my_video_area_set_scale(priv->video_area,
			gtk_spin_button_get_value(button));
	gtk_adjustment_set_value(h,h_max*hr);
	gtk_adjustment_set_value(w,w_max*wr);
	gtk_widget_queue_draw(priv->video_area);
}

gint scale_input_cb (GtkSpinButton *spin_button,
               gdouble       *new_value,
			   MyMain *self){
	gchar *text=g_strdup(gtk_entry_get_text(spin_button));
	gchar *percent=g_strstr_len(text,-1,"%");
	if(percent!=NULL)*percent=0x00;
	*new_value=g_strtod(text,NULL)/100.;
	g_free(text);
	return TRUE;
}

gboolean scale_output_cb(GtkSpinButton *button,MyMain *self){
	gdouble value=gtk_spin_button_get_value(button);
	value*=100.;
	gchar *text=g_strdup_printf("%.2f%%",value);
	gtk_entry_set_text(button,text);
	g_free(text);
	return TRUE;
}

void radio_lock_cb(GtkToggleButton *size_radio_lock, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	if (gtk_adjustment_get_value(priv->size_h) != 0.)
		priv->r_w_h = gtk_adjustment_get_value(priv->size_w)
				/ gtk_adjustment_get_value(priv->size_h);
}

void size_fit_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	if (priv->current_area == NULL)return;
	GdkPixbuf *pixbuf=my_video_area_get_pixbuf(priv->video_area);
	if(pixbuf==NULL)return;
	gint w=gdk_pixbuf_get_width(pixbuf);
	gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->w=w*1.;
	priv->current_area->area->h=h*1.;
	gtk_widget_queue_draw(priv->video_area);
	update_area_info(priv->current_area->area, self);

}

void align_center_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	if (priv->current_area == NULL)return;
	GdkPixbuf *pixbuf=my_video_area_get_pixbuf(priv->video_area);
	if(pixbuf==NULL)return;
	gint w=gdk_pixbuf_get_width(pixbuf);
	gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetX=w/2.;
	priv->current_area->area->offsetY=h/2.;
	priv->current_area->area->obj_mat.x0=0.;
	priv->current_area->area->obj_mat.y0=0.;
	gtk_widget_queue_draw(priv->video_area);
	update_area_info(priv->current_area->area, self);
}

void align_top_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	if (priv->current_area == NULL)return;
	GdkPixbuf *pixbuf=my_video_area_get_pixbuf(priv->video_area);
	if(pixbuf==NULL)return;
	//gint w=gdk_pixbuf_get_width(pixbuf);
	//gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetY=priv->current_area->area->h/2.;
	priv->current_area->area->obj_mat.x0=0.;
	priv->current_area->area->obj_mat.y0=0.;
	gtk_widget_queue_draw(priv->video_area);
	update_area_info(priv->current_area->area, self);
}

void align_right_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	if (priv->current_area == NULL)return;
	GdkPixbuf *pixbuf=my_video_area_get_pixbuf(priv->video_area);
	if(pixbuf==NULL)return;
	gint w=gdk_pixbuf_get_width(pixbuf);
	//gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetX=w*1.-priv->current_area->area->w/2.;
	priv->current_area->area->obj_mat.x0=0.;
	priv->current_area->area->obj_mat.y0=0.;
	gtk_widget_queue_draw(priv->video_area);
	update_area_info(priv->current_area->area, self);
}

void align_left_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	if (priv->current_area == NULL)return;
	GdkPixbuf *pixbuf=my_video_area_get_pixbuf(priv->video_area);
	if(pixbuf==NULL)return;
	//gint w=gdk_pixbuf_get_width(pixbuf);
	//gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetX=priv->current_area->area->w/2.;
	priv->current_area->area->obj_mat.x0=0.;
	priv->current_area->area->obj_mat.y0=0.;
	gtk_widget_queue_draw(priv->video_area);
	update_area_info(priv->current_area->area, self);
}
void align_buttom_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	if (priv->current_area == NULL)return;
	GdkPixbuf *pixbuf=my_video_area_get_pixbuf(priv->video_area);
	if(pixbuf==NULL)return;
	//gint w=gdk_pixbuf_get_width(pixbuf);
	gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetY=h*1.-priv->current_area->area->h/2.;
	priv->current_area->area->obj_mat.x0=0.;
	priv->current_area->area->obj_mat.y0=0.;
	gtk_widget_queue_draw(priv->video_area);
	update_area_info(priv->current_area->area, self);
}

static void my_main_class_init(MyMainClass *klass) {
	size_t s;
	gchar *glade;
	g_file_get_contents("ui/MyMain.glade", &glade, &s, NULL);
	GBytes *b = g_bytes_new(glade, s);
	gtk_widget_class_set_template(klass, b);
	g_bytes_unref(b);
	g_free(glade);
	gtk_widget_class_bind_template_child_private(klass, MyMain, video_box);
	gtk_widget_class_bind_template_child_private(klass, MyMain, pos_x);
	gtk_widget_class_bind_template_child_private(klass, MyMain, pos_y);
	gtk_widget_class_bind_template_child_private(klass, MyMain, progress);
	gtk_widget_class_bind_template_child_private(klass, MyMain, size_w);
	gtk_widget_class_bind_template_child_private(klass, MyMain, size_h);
	gtk_widget_class_bind_template_child_private(klass, MyMain, rotate);
	gtk_widget_class_bind_template_child_private(klass, MyMain, volume);
	gtk_widget_class_bind_template_child_private(klass, MyMain, paned);
	gtk_widget_class_bind_template_child_private(klass, MyMain, modifier_view);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			size_radio_lock);
	gtk_widget_class_bind_template_child_private(klass, MyMain, area_label);

	gtk_widget_class_bind_template_callback(klass, label_changed_cb);
	gtk_widget_class_bind_template_callback(klass, pos_changed_cb);
	gtk_widget_class_bind_template_callback(klass, size_changed_cb);
	gtk_widget_class_bind_template_callback(klass, radio_lock_cb);
	gtk_widget_class_bind_template_callback(klass, rotate_changed_cb);
	gtk_widget_class_bind_template_callback(klass, scale_changed_cb);
	gtk_widget_class_bind_template_callback(klass, scale_output_cb);
	gtk_widget_class_bind_template_callback(klass, scale_input_cb);

	gtk_widget_class_bind_template_callback(klass, size_fit_cb);
	gtk_widget_class_bind_template_callback(klass, align_buttom_cb);
	gtk_widget_class_bind_template_callback(klass, align_center_cb);
	gtk_widget_class_bind_template_callback(klass, align_left_cb);
	gtk_widget_class_bind_template_callback(klass, align_right_cb);
	gtk_widget_class_bind_template_callback(klass, align_top_cb);

}

static void my_main_init(MyMain *self) {
	gint w, h;
	gtk_widget_init_template(self);
	GET_PRIV;
	priv->video_area = my_video_area_new();
	priv->current_area = NULL;
	priv->area_table = g_hash_table_new(g_direct_hash, g_direct_equal);
	gtk_container_add(priv->video_box, priv->video_area);
	//gtk_box_pack_start (priv->video_box, priv->video_area, TRUE, TRUE, 0);
	my_video_area_set_pixbuf(priv->video_area,
			gdk_pixbuf_new_from_file("dog.jpg", NULL));
	gtk_widget_get_size_request(priv->video_area, &w, &h);
	gtk_paned_set_position(priv->paned, w);
	g_signal_connect(priv->video_area, "area_select", area_select, self);
	g_signal_connect(priv->video_area, "area_move", area_move, self);
	g_signal_connect(priv->video_area, "area_resize", area_resize, self);
	g_signal_connect(priv->video_area, "area_rotate", area_rotate, self);
	my_main_add_area(self, "0", 0, 0, 100, 200);
	my_main_add_area(self, "1", 100, 200, 100, 200);
}

void my_main_add_area(MyMain *self, gchar *label, gfloat x, gfloat y, gfloat w,
		gfloat h) {
	GET_PRIV;
	AreaInfo *info = g_malloc(sizeof(AreaInfo));
	info->area = my_video_area_add_area(priv->video_area, label, NULL, x, y, w,
			h);
	info->pixbuf = NULL;
	info->process_list = create_process_list();
	g_hash_table_insert(priv->area_table, info->area, info);
}
