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
	cairo_surface_t *surf;
	GtkListStore *process_list;
} AreaInfo;

typedef struct {
	GtkAdjustment *pos_x, *pos_y, *progress, *rotate, *size_h, *size_w, *volume,
			*scale, *play_speed;
	GtkScrolledWindow *video_box;
	MyVideoArea *video_area;
	GtkTreeView *post_tree_view;
	GtkPaned *paned;
	AreaInfo *current_area;
	GtkToggleButton *size_radio_lock, *full_screen;
	GtkEntry *area_label, *open_uri;
	GHashTable *area_table;
	gdouble r_w_h;
	GstPipeline *pline, *screen_line, *internal_audio_line, *mic_line;
	GstElement *video_sink, *playbin,*tee_sink,*area_filter;
	GstState state;
	GtkDialog *open_uri_dialog;
	GtkDrawingArea *preview_area;
	GtkMenu *add_post_menu;
	GtkImage *bw,*diff,*file,*gray,*remap,*rgbfmt,*scan,*transparent,*window,*image_file,*resize;
} MyMainPrivate;

G_DEFINE_TYPE_WITH_CODE(MyMain, my_main, GTK_TYPE_WINDOW,
		G_ADD_PRIVATE(MyMain));

enum {
	col_id, col_name,col_type_pixbuf,col_type,col_post, col_preview_pixbuf,num_col
};

void post_view_refresh(	MyMain *self) ;
GtkListStore* create_process_list() {
	GtkListStore *list = gtk_list_store_new(num_col, G_TYPE_UINT, G_TYPE_STRING,
	GDK_TYPE_PIXBUF,G_TYPE_UINT,G_TYPE_POINTER,GDK_TYPE_PIXBUF);
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
	if (priv->current_area == NULL)
		return;
	radio = gtk_toggle_button_get_active(priv->size_radio_lock);
	gtk_toggle_button_set_active(priv->size_radio_lock, FALSE);
	update_area_info(area, self);
	priv->r_w_h = area->w / area->h;
	gtk_toggle_button_set_active(priv->size_radio_lock, radio);
	AreaInfo *info = g_hash_table_lookup(priv->area_table, area);
	gtk_tree_view_set_model(priv->post_tree_view, info->process_list);
	post_view_refresh(self);
}

void area_move(MyVideoArea *video_area, VideoBoxArea *area, gdouble *x,
		gdouble *y, MyMain *self) {
	GET_PRIV;
	//update_area_info (area, self);
	gtk_adjustment_set_value(priv->pos_x, area->offsetX);
	gtk_adjustment_set_value(priv->pos_y, area->offsetY);
	post_view_refresh(self);
}

void area_resize(MyVideoArea *video_area, VideoBoxArea *area, gdouble *add_w,
		gdouble *add_h, MyMain *self) {
	GET_PRIV;
	//update_area_info (area, self);
	if (gtk_toggle_button_get_active(priv->size_radio_lock)) {
		gdouble r = priv->r_w_h;
		priv->current_area->area->w -= *add_w;
		priv->current_area->area->h -= *add_h;
		*add_w = r * *add_h;
		priv->current_area->area->w += *add_w;
		priv->current_area->area->h += *add_h;
	}
	gtk_adjustment_set_value(priv->size_w, area->w);
	gtk_adjustment_set_value(priv->size_h, area->h);
	post_view_refresh(self);
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
	post_view_refresh(self);
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
	gtk_widget_queue_draw(priv->preview_area);
	return;
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
	gtk_widget_queue_draw(priv->preview_area);
	return;
}

void rotate_changed_cb(GtkSpinButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	gdouble value = gtk_spin_button_get_value(button);
	value = value * -1. * M_PI / 180.;
	cairo_matrix_t rotate;
	cairo_matrix_init_rotate(&rotate, value);
	priv->current_area->area->obj_mat.xx = rotate.xx;
	priv->current_area->area->obj_mat.xy = rotate.xy;
	priv->current_area->area->obj_mat.yy = rotate.yy;
	priv->current_area->area->obj_mat.yx = rotate.yx;
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
	return;
}

void scale_changed_cb(GtkSpinButton *button, MyMain *self) {
	GET_PRIV;
	//gtk_toggle_button_set_active(priv->full_screen,FALSE);
	gdouble h_max, w_max;
	GtkAdjustment *h = gtk_scrolled_window_get_hadjustment(priv->video_box);
	GtkAdjustment *w = gtk_scrolled_window_get_vadjustment(priv->video_box);
	h_max = gtk_adjustment_get_upper(h);
	w_max = gtk_adjustment_get_upper(w);
	gdouble hr = gtk_adjustment_get_value(h);
	gdouble wr = gtk_adjustment_get_value(w);
	gdouble old_scale = my_video_area_get_scale(priv->video_area);
	gdouble new_scale = gtk_spin_button_get_value(button);
	if (h_max != 0)
		hr /= h_max;
	//old_scale=h_max/gdk_pixbuf_get_height(my_video_area_get_pixbuf(priv->video_area));
	else
		hr = 0.5;
	if (w_max != 0)
		wr /= w_max;
	else
		wr = 0.5;
	my_video_area_set_scale(priv->video_area, new_scale);
	gtk_adjustment_set_value(h, h_max * hr * new_scale / old_scale);
	gtk_adjustment_set_value(w, w_max * wr * new_scale / old_scale);
	gtk_widget_queue_draw(priv->video_area);
}

gint scale_input_cb(GtkSpinButton *spin_button, gdouble *new_value,
		MyMain *self) {
	gchar *text = g_strdup(gtk_entry_get_text(spin_button));
	gchar *percent = g_strstr_len(text, -1, "%");
	if (percent != NULL)
		*percent = 0x00;
	*new_value = g_strtod(text, NULL) / 100.;
	g_free(text);
	return TRUE;
}

gboolean scale_output_cb(GtkSpinButton *button, MyMain *self) {
	gdouble value = gtk_spin_button_get_value(button);
	value *= 100.;
	gchar *text = g_strdup_printf("%.2f%%", value);
	gtk_entry_set_text(button, text);
	g_free(text);
	return TRUE;
}

void play_speed_cb(GtkSpinButton *button, MyMain *self) {
	GET_PRIV;
	gint64 pos;
	GstEvent *event = NULL;
	gdouble speed = gtk_spin_button_get_value(button);
	gst_element_query_position(priv->pline, GST_FORMAT_TIME, &pos);
	if (speed == 0.) {
		gst_element_set_state(priv->pline, GST_STATE_PAUSED);
		return;
	} else if (speed > 0.) {
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				pos, GST_SEEK_TYPE_END, 0);
	} else { //speed<0.
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				0, GST_SEEK_TYPE_SET, pos);
	}
	//gst_element_set_state(priv->pline,GST_STATE_PLAYING);
	gst_element_send_event(priv->pline, event);
}

gint play_speed_input_cb(GtkSpinButton *spin_button, gdouble *new_value,
		MyMain *self) {
	gchar *text = g_strdup(gtk_entry_get_text(spin_button));
	gchar *percent = g_strstr_len(text, -1, "x");
	if (percent != NULL)
		*percent = ' ';
	*new_value = g_strtod(text, NULL);
	g_free(text);
	return TRUE;
}
gboolean play_speed_output_cb(GtkSpinButton *button, MyMain *self) {
	gdouble value = gtk_spin_button_get_value(button);
	gchar *text = g_strdup_printf("x%.2f", value);
	gtk_entry_set_text(button, text);
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

void size_fit_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	GdkPixbuf *pixbuf = my_video_area_get_pixbuf(priv->video_area);
	if (pixbuf == NULL)
		return;
	gint w = gdk_pixbuf_get_width(pixbuf);
	gint h = gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->w = w * 1.;
	priv->current_area->area->h = h * 1.;
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
	update_area_info(priv->current_area->area, self);

}

void align_center_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	GdkPixbuf *pixbuf = my_video_area_get_pixbuf(priv->video_area);
	if (pixbuf == NULL)
		return;
	gint w = gdk_pixbuf_get_width(pixbuf);
	gint h = gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetX = w / 2.;
	priv->current_area->area->offsetY = h / 2.;
	priv->current_area->area->obj_mat.x0 = 0.;
	priv->current_area->area->obj_mat.y0 = 0.;
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
	update_area_info(priv->current_area->area, self);
}

void align_top_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	GdkPixbuf *pixbuf = my_video_area_get_pixbuf(priv->video_area);
	if (pixbuf == NULL)
		return;
	//gint w=gdk_pixbuf_get_width(pixbuf);
	//gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetY = priv->current_area->area->h / 2.;
	priv->current_area->area->obj_mat.x0 = 0.;
	priv->current_area->area->obj_mat.y0 = 0.;
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
	update_area_info(priv->current_area->area, self);
}

void align_right_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	GdkPixbuf *pixbuf = my_video_area_get_pixbuf(priv->video_area);
	if (pixbuf == NULL)
		return;
	gint w = gdk_pixbuf_get_width(pixbuf);
	//gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetX = w * 1.
			- priv->current_area->area->w / 2.;
	priv->current_area->area->obj_mat.x0 = 0.;
	priv->current_area->area->obj_mat.y0 = 0.;
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
	update_area_info(priv->current_area->area, self);
}

void align_left_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	GdkPixbuf *pixbuf = my_video_area_get_pixbuf(priv->video_area);
	if (pixbuf == NULL)
		return;
	//gint w=gdk_pixbuf_get_width(pixbuf);
	//gint h=gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetX = priv->current_area->area->w / 2.;
	priv->current_area->area->obj_mat.x0 = 0.;
	priv->current_area->area->obj_mat.y0 = 0.;
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
	update_area_info(priv->current_area->area, self);
}
void align_buttom_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	GdkPixbuf *pixbuf = my_video_area_get_pixbuf(priv->video_area);
	if (pixbuf == NULL)
		return;
	//gint w=gdk_pixbuf_get_width(pixbuf);
	gint h = gdk_pixbuf_get_height(pixbuf);
	priv->current_area->area->offsetY = h * 1.
			- priv->current_area->area->h / 2.;
	priv->current_area->area->obj_mat.x0 = 0.;
	priv->current_area->area->obj_mat.y0 = 0.;
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
	update_area_info(priv->current_area->area, self);
}

void volume_changed_cb(GtkScaleButton *button, MyMain *self) {
	GET_PRIV;
	gdouble v = gtk_adjustment_get_value(priv->volume);
	g_object_set(priv->playbin, "volume", v, NULL);
}

void play_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	gst_element_set_state(priv->pline, GST_STATE_PLAYING);
	priv->state = GST_STATE_PLAYING;
}

void pause_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	gst_element_set_state(priv->pline, GST_STATE_PAUSED);
	priv->state = GST_STATE_PAUSED;
}

gboolean progess_changed_cb(GtkScale *scale, GtkScrollType scroll,
		gdouble value, MyMain *self) {
	GET_PRIV;
	GstEvent *event;
	gdouble speed = gtk_adjustment_get_value(priv->play_speed);
	gint64 pos = value * GST_SECOND;
	gst_element_set_state(priv->pline, GST_STATE_PLAYING);
	if (speed >= 0.) {
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				pos, GST_SEEK_TYPE_END, 0);
	} else {	//speed<0.
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				0, GST_SEEK_TYPE_SET, pos);
	}
	gst_element_send_event(priv->pline, event);
	gst_element_set_state(priv->pline, priv->state);
	return TRUE;
}

void open_file_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	GtkFileChooserDialog *dialog = gtk_file_chooser_dialog_new(
			"Open Video File", self, GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_OPEN, GTK_RESPONSE_OK, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, NULL);
	if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
		gchar *uri = gtk_file_chooser_get_uri(dialog);
		if (uri != NULL) {
			gst_element_set_state(priv->pline, GST_STATE_NULL);
			g_object_set(priv->playbin, "uri", uri, NULL);
			g_free(uri);
			gst_element_set_state(priv->pline, priv->state);
		}
	}
	gtk_widget_destroy(dialog);
}

void open_uri_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	gchar *uri;
	g_object_get(priv->playbin, "uri", &uri, NULL);
	gtk_entry_set_text(priv->open_uri, uri);
	g_free(uri);
	if (gtk_dialog_run(priv->open_uri_dialog) == GTK_RESPONSE_OK) {
		gst_element_set_state(priv->pline, GST_STATE_NULL);
		uri = gtk_entry_get_text(priv->open_uri);
		g_object_set(priv->playbin, "uri", uri, NULL);
		gst_element_set_state(priv->pline, priv->state);
	}
	gtk_widget_hide(priv->open_uri_dialog);
}

void open_screen_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	g_print("open screen");

}

void add_area_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	my_main_add_area(self, NULL, 100, 112, 100, 100);
}

void remove_area_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area != NULL) {
		my_main_remove_area(self, priv->current_area->area);
		priv->current_area = NULL;
	}
}

gboolean preview_area_draw_cb(GtkDrawingArea *preview_area, cairo_t *cr,
		MyMain *self) {
	GET_PRIV;
	GtkAllocation alloc;
	gtk_widget_get_allocation(preview_area, &alloc);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_fill(cr);
	if(priv->current_area == NULL)return TRUE;
		if (priv->current_area->surf != NULL)
			cairo_surface_destroy(priv->current_area->surf);
		priv->current_area->surf = my_video_area_get_area_content(priv->video_area, priv->current_area->area);
		if (priv->current_area->surf == NULL)return TRUE;
		gdouble w, h, wr, hr;
		w = cairo_image_surface_get_width(priv->current_area->surf);
		h = cairo_image_surface_get_height(priv->current_area->surf);
		wr = alloc.width / w;
		hr = alloc.height / h;
		wr = wr < hr ? wr : hr;
		cairo_save(cr);
		cairo_translate(cr, alloc.width/2., 0.);
		cairo_scale(cr, wr, wr);
		cairo_set_source_surface(cr, priv->current_area->surf, w/-2., 0);
		if(wr>1.)cairo_pattern_set_filter( cairo_get_source(cr),CAIRO_FILTER_NEAREST);//enlarge the image
		cairo_paint(cr);
		cairo_restore(cr);
	return TRUE;
}

void post_view_refresh(	MyMain *self) {
	GET_PRIV;
	GtkTreeIter iter;
	GdkPixbuf *new,*old;
	GObject *obj;
	PostCommon *post;
	if(priv->current_area == NULL)return;
	gboolean not_empty=gtk_tree_model_get_iter_first(priv->current_area->process_list,&iter);
	if(!not_empty)return;
	cairo_surface_t *preview_surf=my_video_area_get_area_content(priv->video_area, priv->current_area->area);
	if(preview_surf==NULL)return;
	while(1){
		gtk_tree_model_get(priv->current_area->process_list,&iter,col_post,&post,col_preview_pixbuf,&old,-1);
		new=post_preview(post, preview_surf);
		gtk_list_store_set(priv->current_area->process_list,&iter,col_preview_pixbuf,new,-1);
		cairo_surface_destroy(preview_surf);
		if(old!=NULL)g_object_unref(old);
		if(!gtk_tree_model_iter_next(priv->current_area->process_list,&iter)){
			g_object_unref(new);
			break;
		}
		preview_surf=gdk_cairo_surface_create_from_pixbuf(new,1,NULL);
		g_object_unref(new);
	}
}


void full_screen(MyMain *self) {
	GET_PRIV;
	GdkPixbuf *p = my_video_area_get_pixbuf(priv->video_area);
	if (p == NULL)
		return;
	gint w = gdk_pixbuf_get_width(p);
	gint h = gdk_pixbuf_get_height(p);
	GtkAllocation alloc;
	gtk_widget_get_allocation(priv->video_box, &alloc);
	gdouble wr = alloc.width / (gdouble) w;
	gdouble hr = alloc.height / (gdouble) h;
	gtk_adjustment_set_value(priv->scale, wr < hr ? wr : hr);
}

void full_screen_toggled_cb(GtkToggleButton *togglebutton, MyMain *self) {
	if (gtk_cell_renderer_toggle_get_active(togglebutton)) {
		full_screen(self);
	}
}

void add_post_menu_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	gtk_menu_popup_at_widget(priv->add_post_menu,button,GDK_GRAVITY_NORTH,GDK_GRAVITY_NORTH,NULL);
}

void del_post_cb(GtkButton *button,MyMain *self){
	GET_PRIV;
	GtkTreeIter iter;
	GtkTreePath *path;
	PostCommon *post;
	gboolean b;
	if(priv->current_area==NULL)return;
	GtkTreeSelection *sel=gtk_tree_view_get_selection(priv->post_tree_view);
	GtkListStore *list_store=priv->current_area->process_list;
	GList *l,*list_row=NULL,*list=gtk_tree_selection_get_selected_rows(sel,&list_store);
	l=list;
	while(l!=NULL){
		list_row=g_list_append(list_row,gtk_tree_row_reference_new(list_store,l->data));
		l=l->next;
	}
	g_list_free_full(list,gtk_tree_path_free);

	l=list_row;
	while(l!=NULL){
		path=gtk_tree_row_reference_get_path(l->data);
		gtk_tree_model_get_iter(list_store,&iter,path);
		gtk_tree_model_get(list_store,&iter,col_post,&post,-1);
		post_free(post);
		gtk_list_store_remove(list_store,&iter);
		gtk_tree_path_free(path);
		l=l->next;
	}
	g_list_free_full(list_row,gtk_tree_row_reference_free);
}

void post_name_changed_cb (GtkCellRendererText *renderer,
               gchar               *path,
               gchar               *new_text,
			   MyMain *self){
	GET_PRIV;
	GtkTreePath *p=gtk_tree_path_new_from_string(path);
	GtkListStore *store=priv->current_area->process_list;
	GtkTreeIter iter;
	gtk_tree_model_get_iter(store,&iter,p);
	gtk_list_store_set(store,&iter,col_name,new_text,-1);
	gtk_tree_path_free(p);
}

void add_post_process(MyMain *self,PostCommon *post,gchar *name,PostType type){
	GET_PRIV;
	GtkTreeIter iter;
	AreaInfo *info=priv->current_area;
	post->post_type=type;
	guint id=gtk_tree_model_iter_n_children(info->process_list,NULL);
	gchar *n=g_strdup_printf("%s %d",name,id);
	gtk_list_store_append(info->process_list,&iter);
	GtkImage *ti=NULL;
	GdkPixbuf *pixbuf=NULL;
	switch (type){
	case POST_ARGB_REMAP:
		ti=priv->remap;
		break;
	case POST_BITMAP:
		ti=priv->scan;
		break;
	case POST_BW:
		ti=priv->bw;
		break;
	case POST_DIFFUSE:
		ti=priv->diff;
		break;
	case POST_GRAY:
		ti=priv->gray;
		break;
	case POST_RESIZE:
		ti=priv->resize;
		break;
	case POST_RGB_FMT:
		ti=priv->rgbfmt;
		break;
	case POST_TRANSPARENT:
		ti=priv->transparent;
		break;
	case OUT_FILE:
		ti=priv->file;
		break;
	case OUT_IMG_FILE:
		ti=priv->image_file;
		break;
	case OUT_WINDOWS:
		ti=priv->window;
		break;
	default:
		ti=NULL;
	    break;
	}
	if(ti!=NULL)pixbuf=gtk_image_get_pixbuf(ti);
	gtk_list_store_set(info->process_list,&iter,col_post,post,col_id,id,col_name,n,col_preview_pixbuf,NULL,col_type_pixbuf,pixbuf,col_type,type,-1);
	g_free(n);
}

void add_to_gray_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostGray *post=g_malloc0(sizeof(PostGray));
	add_post_process(self, post, "Gray", POST_GRAY);
}

void add_to_bw_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostBw *post=g_malloc0(sizeof(PostBw));
	add_post_process(self, post, "Black White", POST_BW);
	post->thresold=127;

}

void add_argb_remap_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostARGBRemap *post=g_malloc0(sizeof(PostARGBRemap));
	add_post_process(self, post, "ARGB Remap", POST_ARGB_REMAP);
	post->remap_weight.AA=1;
	post->remap_weight.RR=1;
	post->remap_weight.GG=1;
	post->remap_weight.BB=1;
}

void add_error_diffuse_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostDiffuse *post=g_malloc0(sizeof(PostDiffuse));
	add_post_process(self, post, "Error Diffusion", POST_DIFFUSE);
	post->rank=2;
	post->radio.bm=diff_332.bm;
	post->radio.r=diff_332.r;
	post->radio.rb=diff_332.rb;
}

void add_transparent_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostTransparent *post=g_malloc0(sizeof(PostTransparent));
	add_post_process(self, post, "Transparent", POST_TRANSPARENT);
	post->a=255;
	post->color_distance=10.;
}

void add_rgb_data_format_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostRGBFmt *post=g_malloc0(sizeof(PostRGBFmt));
	add_post_process(self, post, "RGB Format", POST_RGB_FMT);
	post->fmt=RGB_FORMAT_888;
}

void add_scan_bitmap_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostBitmap *post=g_malloc0(sizeof(PostBitmap));
	add_post_process(self, post, "Scan Bitmap", POST_BITMAP);
	post->thresold=127;
	post->gray_rank=2;
}

void add_resize_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostResize *post=g_malloc0(sizeof(PostResize));
	add_post_process(self, post, "Resize", POST_RESIZE);
	post->resize_w=128;
	post->resize_h=64;
}

void add_output_window_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	PostCommon *post=g_malloc0(sizeof(OutWindow));
	add_post_process(self, post, "Window Output", OUT_WINDOWS);
}

void add_output_raw_file_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	OutFile *post=g_malloc0(sizeof(OutFile));
	add_post_process(self, post, "File Output", OUT_FILE);
	post->filename=g_strdup("output.txt");
}

void add_output_image_file_cb (GtkMenuItem *menuitem,MyMain *self){
	GET_PRIV;
	if(priv->current_area==NULL)return;
	OutImgFile *post=g_malloc0(sizeof(OutImgFile));
	add_post_process(self, post, "Image Output", OUT_IMG_FILE);
	post->name_fmt=g_strdup("image-%d.png");
}


gboolean message_watch_cb(GstBus *bus, GstMessage *message, MyMain *self) {
	GET_PRIV;
	gint64 pos, dur;
	if (message->type == GST_MESSAGE_ELEMENT) {
		GstStructure *s = gst_message_get_structure(message);
		if (gst_structure_has_field_typed(s, "pixbuf", GDK_TYPE_PIXBUF)) {
			GdkPixbuf *p = NULL;
			gst_structure_get(s, "pixbuf", GDK_TYPE_PIXBUF, &p, NULL);
			my_video_area_set_pixbuf(priv->video_area, p);
			gtk_widget_queue_draw(priv->video_area);
			post_view_refresh(self);
			g_object_unref(p);
			gst_element_query_position(priv->pline, GST_FORMAT_TIME, &pos);
			gst_element_query_duration(priv->pline, GST_FORMAT_TIME, &dur);
			gtk_adjustment_set_upper(priv->progress, dur / GST_SECOND);
			gtk_adjustment_set_value(priv->progress, pos / GST_SECOND);
			if (gtk_toggle_button_get_active(priv->full_screen)) {
				full_screen(self);
			}
		};
	}
//	GstPad *pad=gst_element_get_static_pad(priv->video_sink,"sink");
//	GstCaps *caps=gst_pad_query_caps(pad,NULL);
//	if(caps==NULL){
//		g_object_unref(pad);
//		return G_SOURCE_CONTINUE;
//	}
//	gchar *s=gst_caps_to_string(caps);
//	g_print("%s\n",s);
//	g_free(s);
//	gst_caps_unref(caps);
//	g_object_unref(pad);
	return G_SOURCE_CONTINUE;
}

static void my_main_class_init(MyMainClass *klass) {
//	size_t s;
//	gchar *glade;
//	g_file_get_contents("ui/MyMain.glade", &glade, &s, NULL);
//	GBytes *b = g_bytes_new(glade, s);
//	gtk_widget_class_set_template(klass, b);
//	g_bytes_unref(b);
//	g_free(glade);
	gtk_widget_class_set_template_from_resource(klass,"/my/MyMain.glade");
	gtk_widget_class_bind_template_child_private(klass, MyMain, video_box);
	gtk_widget_class_bind_template_child_private(klass, MyMain, pos_x);
	gtk_widget_class_bind_template_child_private(klass, MyMain, pos_y);
	gtk_widget_class_bind_template_child_private(klass, MyMain, progress);
	gtk_widget_class_bind_template_child_private(klass, MyMain, size_w);
	gtk_widget_class_bind_template_child_private(klass, MyMain, size_h);
	gtk_widget_class_bind_template_child_private(klass, MyMain, rotate);
	gtk_widget_class_bind_template_child_private(klass, MyMain, volume);
	gtk_widget_class_bind_template_child_private(klass, MyMain, paned);
	gtk_widget_class_bind_template_child_private(klass, MyMain, post_tree_view);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			size_radio_lock);
	gtk_widget_class_bind_template_child_private(klass, MyMain, area_label);
	gtk_widget_class_bind_template_child_private(klass, MyMain, scale);
	gtk_widget_class_bind_template_child_private(klass, MyMain, play_speed);
	gtk_widget_class_bind_template_child_private(klass, MyMain, full_screen);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			open_uri_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain, open_uri);
	gtk_widget_class_bind_template_child_private(klass, MyMain, preview_area);
	gtk_widget_class_bind_template_child_private(klass, MyMain, add_post_menu);

	gtk_widget_class_bind_template_child_private(klass, MyMain, bw);
	gtk_widget_class_bind_template_child_private(klass, MyMain, diff);
	gtk_widget_class_bind_template_child_private(klass, MyMain, file);
	gtk_widget_class_bind_template_child_private(klass, MyMain, gray);
	gtk_widget_class_bind_template_child_private(klass, MyMain, remap);
	gtk_widget_class_bind_template_child_private(klass, MyMain, rgbfmt);
	gtk_widget_class_bind_template_child_private(klass, MyMain, scan);
	gtk_widget_class_bind_template_child_private(klass, MyMain, transparent);
	gtk_widget_class_bind_template_child_private(klass, MyMain, window);
	gtk_widget_class_bind_template_child_private(klass, MyMain, image_file);
	gtk_widget_class_bind_template_child_private(klass, MyMain, resize);


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

	gtk_widget_class_bind_template_callback(klass, play_speed_cb);
	gtk_widget_class_bind_template_callback(klass, play_speed_input_cb);
	gtk_widget_class_bind_template_callback(klass, play_speed_output_cb);
	gtk_widget_class_bind_template_callback(klass, volume_changed_cb);
	gtk_widget_class_bind_template_callback(klass, play_cb);
	gtk_widget_class_bind_template_callback(klass, pause_cb);
	gtk_widget_class_bind_template_callback(klass, progess_changed_cb);

	gtk_widget_class_bind_template_callback(klass, remove_area_cb);
	gtk_widget_class_bind_template_callback(klass, add_area_cb);
	gtk_widget_class_bind_template_callback(klass, open_file_cb);
	gtk_widget_class_bind_template_callback(klass, open_uri_cb);
	gtk_widget_class_bind_template_callback(klass, open_screen_cb);
	gtk_widget_class_bind_template_callback(klass, full_screen_toggled_cb);
	gtk_widget_class_bind_template_callback(klass, preview_area_draw_cb);
	gtk_widget_class_bind_template_callback(klass, add_post_menu_cb);
	gtk_widget_class_bind_template_callback(klass, post_name_changed_cb);
	gtk_widget_class_bind_template_callback(klass, del_post_cb);

	//menu callback
	gtk_widget_class_bind_template_callback(klass, add_argb_remap_cb);
	gtk_widget_class_bind_template_callback(klass, add_error_diffuse_cb);
	gtk_widget_class_bind_template_callback(klass, add_output_image_file_cb);
	gtk_widget_class_bind_template_callback(klass, add_output_raw_file_cb);
	gtk_widget_class_bind_template_callback(klass, add_output_window_cb);
	gtk_widget_class_bind_template_callback(klass, add_rgb_data_format_cb);
	gtk_widget_class_bind_template_callback(klass, add_scan_bitmap_cb);
	gtk_widget_class_bind_template_callback(klass, add_to_bw_cb);
	gtk_widget_class_bind_template_callback(klass, add_to_gray_cb);
	gtk_widget_class_bind_template_callback(klass, add_transparent_cb);
	gtk_widget_class_bind_template_callback(klass, add_resize_cb);

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
	gtk_paned_set_position(priv->paned, 550);
	g_signal_connect(priv->video_area, "area_select", area_select, self);
	g_signal_connect(priv->video_area, "area_move", area_move, self);
	g_signal_connect(priv->video_area, "area_resize", area_resize, self);
	g_signal_connect(priv->video_area, "area_rotate", area_rotate, self);

	GstBin *bin=gst_bin_new("tee");
	priv->tee_sink=bin;
	GstElement *tee=gst_element_factory_make("tee","tee");
	gst_bin_add(bin,tee);
	GstPad *src_pad,*sink_pad;
	sink_pad=gst_element_get_static_pad(tee,"sink");
	gst_element_add_pad(bin,gst_ghost_pad_new("sink",sink_pad));
	gst_object_unref(sink_pad);


	priv->video_sink = gst_element_factory_make("gdkpixbufsink", "sink");
	gst_bin_add(bin,priv->video_sink);
	src_pad=gst_element_get_request_pad(tee,"src_0");
	sink_pad=gst_element_get_static_pad(priv->video_sink,"sink");
	gst_pad_link(src_pad,sink_pad);
	gst_object_unref(src_pad);
	gst_object_unref(sink_pad);

//	priv->area_filter=g_object_new(MY_TYPE_VIDEO_AREA_FILTER,NULL);//gst_element_factory_make("videoareafilter","videoareafilter");
//	gst_bin_add(bin,priv->area_filter);
//	sink_pad=gst_element_get_static_pad(priv->area_filter,"sink");
//	src_pad=gst_element_get_request_pad(tee,"src_1");
//	gst_pad_link(src_pad,sink_pad);
//	gst_object_unref(src_pad);
//	gst_object_unref(sink_pad);

	priv->playbin = gst_element_factory_make("playbin", "playbin");
	priv->pline = gst_pipeline_new("line");
	gst_bin_add_many(priv->pline, priv->playbin, NULL);
	//g_object_set(priv->playbin,"video-sink",priv->video_sink,"uri","file:///home/tom/eclipse-workspace/Spring.mp4",NULL);
	g_object_set(priv->playbin, "video-sink", priv->tee_sink, "uri",
			"file:///home/tom/dwhelper/2020VSINGERLIVE%E6%BC%94%E5%94%B1%E4%BC%9A%E8%AF%A6%E6%83%85%E4%BB%8B%E7%BB%8D-2020VSINGERLIVE%E6%BC%94%E5%94%B1%E4%BC%9A%E5%9C%A8%E7%BA%BF%E8%A7%82%E7%9C%8B-2020VSINGERLIV.mp4",
			"volume",gtk_adjustment_get_value(priv->volume),
			NULL);
	gst_bus_add_watch(gst_pipeline_get_bus(priv->pline), message_watch_cb,
			self);
	my_main_add_area(self, "1", 100, 200, 100, 200);
	gst_element_set_state(priv->pline, GST_STATE_PLAYING);
	priv->state = GST_STATE_PLAYING;
}

MyMain* my_main_new() {
	return g_object_new(MY_TYPE_MAIN, NULL);
}

void my_main_add_area(MyMain *self, gchar *label, gfloat x, gfloat y, gfloat w,
		gfloat h) {
	GET_PRIV;
	AreaInfo *info = g_malloc(sizeof(AreaInfo));
	info->area = my_video_area_add_area(priv->video_area, label, NULL, x, y, w,
			h);
	info->surf = NULL;
	info->process_list = create_process_list();
	g_hash_table_insert(priv->area_table, info->area, info);

//	static int i=0;
//	gchar *name=g_strdup_printf("src_%d",i);
//	MyAreaFilterPad *src_pad=gst_element_get_request_pad(priv->area_filter,name);
//	g_free(name);
//	name=g_strdup_printf("gtksink_%d",i);
//	GstElement *sink=gst_element_factory_make("gtksink",name);
//	g_free(name);
//	i++;
//	gst_bin_add(priv->tee_sink,sink);
//	g_object_set(src_pad,"boxarea",info->area,NULL);
//	GstPad *sink_pad=gst_element_get_static_pad(sink,"sink");
//	gst_pad_link(src_pad,sink_pad);
//	gst_object_unref(src_pad);
//	gst_object_unref(sink_pad);
}

void my_main_remove_area(MyMain *self, VideoBoxArea *area) {
	GET_PRIV;
	GtkTreeIter iter;
	PostCommon *post;
	GObject *preview;
	AreaInfo *info = NULL;
	GtkListStore *liststore = gtk_tree_view_get_model(priv->post_tree_view);
	info = g_hash_table_lookup(priv->area_table, area);
	if (info != NULL) {
		g_hash_table_remove(priv->area_table, area);
		my_video_area_remove_area(priv->video_area, area->label);
		if (info->surf != NULL)
			cairo_surface_destroy(info->surf);
		if (liststore == info->process_list)
			gtk_tree_view_set_model(priv->post_tree_view, NULL);
		if(gtk_tree_model_get_iter_first(liststore,&iter)){
			do{
				gtk_tree_model_get(liststore,&iter,col_preview_pixbuf,&preview,col_post,&post,-1);
				gtk_list_store_set(liststore,&iter,col_preview_pixbuf,NULL,col_post,NULL,-1);
				if(post!=NULL)post_free(post);
				if(preview!=NULL)g_object_unref(preview);
			}while(gtk_tree_model_iter_next(liststore,&iter));
		}
		g_object_unref(info->process_list);
		g_free(info);
	}
}
