/*
 * MyMain.c
 *
 *  Created on: 2021年7月1日
 *      Author: TOM
 *      ⚙☕
 */

#include "MyMain.h"
#include "gresources.h"
#define GET_PRIV   MyMainPrivate *priv=my_main_get_instance_private(self)

typedef struct {
	VideoBoxArea *area;
	cairo_surface_t *surf;
	GtkListStore *process_list;
} AreaInfo;

typedef struct {
	gboolean refresh_preview;
	guint framerate_n, framerate_d;
	GtkAdjustment *pos_x, *pos_y, *progress, *rotate, *size_h, *size_w, *volume,
			*scale, *play_speed;
	GtkScrolledWindow *video_box;
	MyVideoArea *video_area;
	GtkTreeView *post_tree_view;
	GtkPaned *paned;
	AreaInfo *current_area;
	GtkButton *del_post, *fit_size;
	GtkMenuItem *remove;
	GtkToggleButton *size_radio_lock, *full_screen, *run_post;
	GtkEntry *area_label, *open_uri;
	GHashTable *area_table;
	gdouble r_w_h;
	GstPipeline *play_line, *screen_line, *current_line;
	GstElement *video_sink, *playbin, *tee_sink, *area_filter;
	GstState state;
	GtkDrawingArea *preview_area;
	GtkMenu *add_post_menu, *post_tree_view_menu;
	GtkImage *bw, *diff, *file, *gray, *remap, *rgbfmt, *scan, *transparent,
			*window, *image_file, *resize, *scan_preview, *framerate,*image_play;
	GtkDialog *open_uri_dialog;
	GHashTable *thread_table;
	gchar *open_file_name;

//	GThread *thread;
	GAsyncQueue *widget_draw_queue;
	guint64 duration;
	guint64 position;
	gboolean image_refresh;
	//post process setting widget below
	GtkDialog *out_file_dialog, *out_image_file_dialog;
	GtkDialog *post_framerate_dialog;
	GtkDialog *post_bitmap_dialog, *post_bw_dialog, *post_diffuse_dialog,
			*post_gray_dialog, *post_resize_dialog, *post_rgb_fmt_dialog,
			*post_argb_remap_dialog, *post_transparent_dialog;

	//post_argb_remap_dialog
	GtkEntry *AA, *AR, *AG, *AB, *AC;
	GtkEntry *RA, *RR, *RG, *RB, *RC;
	GtkEntry *GA, *GR, *GG, *GB, *GC;
	GtkEntry *BA, *BR, *BG, *BB, *BC;

	//post_bitmap_dialog
	GtkSpinButton *post_bitmap_bw_thresold, *post_bitmap_gray_rank;
	GtkComboBoxText *post_bitmap_mean, *post_bitmap_1st_dir,
			*post_bitmap_2nd_dir, *post_bitmap_bit_dir, *post_bitmap_bit_order,
			*post_bitmap_gray_sim,*post_bitmap_diff_weight;
	GtkImage *post_bitmap_preview, *post_bitmap_bit_dir_preview;

	//post_bw_dialog
	GtkSpinButton *post_bw_thresold;
	GtkComboBoxText *post_bw_mean;

	//post_diffuse_dialog
	GtkSpinButton *post_diffuse_rank;
	GtkEntry *post_diffuse_top_left,*post_diffuse_top_middle,*post_diffuse_right,*post_diffuse_top_right,*post_diffuse_left,*post_diffuse_bottom_left,*post_diffuse_bottom_middle,*post_diffuse_bottom_right;

	//post_gray_dialog
	GtkComboBoxText *post_gray_mean;

	//post_resize_dialog
	GtkSpinButton *post_resize_w, *post_resize_h;
	GtkCheckButton *post_resize_expand, *post_resize_full;

	//post_rgb_fmt_dialog
	GtkComboBoxText *post_rgb_fmt;

	//post_transparent_dialog
	GtkSpinButton *post_transparent_color_distance;
	GtkColorButton *post_transparent_color;

	//post_framerate_dialog
	GtkSpinButton *post_framerate_n, *post_framerate_d;

	//out_image_file_dialog
	GtkEntry *out_image_file_fmt;
	GtkFileChooser *out_image_directory;
	GtkCheckButton *out_image_overwrite;

	//out_file_dialog
	GtkEntry *out_file_filename;
	GtkComboBoxText *out_file_format;
	GtkCheckButton *out_file_output_head, *out_file_overwrite;

} MyMainPrivate;

typedef struct {
	GMutex m;
	gboolean stop;
	gboolean start;
	GAsyncQueue *queue;
	GThread *thread;
	GHashTable *areatable;
} PostThreadData;

void run_post_cb(GtkToggleButton *button, MyMain *self);

void post_list_free(GList *list) {
	g_list_foreach(list, post_stop, NULL);
	g_list_free(list);
}

PostThreadData* post_thread_data_new() {
	PostThreadData *data = malloc(sizeof(PostThreadData));
	g_mutex_init(&data->m);
	data->queue = g_async_queue_new();
	data->areatable = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
			g_list_free);
	data->start = TRUE;
	data->stop = FALSE;
	data->thread = NULL;
	return data;
}

void post_thread_data_free(PostThreadData *data) {
	g_async_queue_unref(data->queue);
	g_hash_table_unref(data->areatable);
	g_free(data);
}

G_DEFINE_TYPE_WITH_CODE(MyMain, my_main, GTK_TYPE_WINDOW,
		G_ADD_PRIVATE(MyMain));

enum {
	col_id,
	col_name,
	col_type_pixbuf,
	col_type,
	col_post,
	col_preview_pixbuf,
	num_col
};

void post_view_refresh(MyMain *self);
GtkListStore* create_process_list() {
	GtkListStore *list = gtk_list_store_new(num_col, G_TYPE_UINT, G_TYPE_STRING,
	GDK_TYPE_PIXBUF, G_TYPE_UINT, G_TYPE_POINTER, GDK_TYPE_PIXBUF);
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
	gboolean runing = g_hash_table_contains(priv->thread_table, info);
	g_signal_handlers_disconnect_by_data(priv->run_post, self);
	gtk_toggle_button_set_active(priv->run_post, runing);
	g_signal_connect(priv->run_post, "toggled", run_post_cb, self);
	priv->refresh_preview=!runing;
}

void area_move(MyVideoArea *video_area, VideoBoxArea *area, gdouble *x,
		gdouble *y, MyMain *self) {
	GET_PRIV;
	//update_area_info (area, self);
	gtk_adjustment_set_value(priv->pos_x, area->offsetX);
	gtk_adjustment_set_value(priv->pos_y, area->offsetY);
	post_view_refresh(self);
}

void area_resize(MyVideoArea *video_area, VideoBoxArea *area, gdouble add_w,
		gdouble add_h, MyMain *self) {
	GET_PRIV;
	//update_area_info (area, self);
	if (gtk_toggle_button_get_active(priv->size_radio_lock)) {
		gdouble r = priv->r_w_h;
		area->w += r * add_h - add_w;
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
	gst_element_query_position(priv->play_line, GST_FORMAT_TIME, &pos);
	if (speed == 0.)
		speed = 0.001;
	if (speed > 0.) {
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				pos, GST_SEEK_TYPE_END, 0);
	} else { //speed<0.
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				0, GST_SEEK_TYPE_SET, pos);
	}
	//gst_element_set_state(priv->play_line,GST_STATE_PLAYING);
	gst_element_send_event(priv->play_line, event);
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
	priv->current_area->area->h=h*1.;
	priv->current_area->area->w=w*1.;
	update_area_info(priv->current_area->area, self);
	//gtk_adjustment_set_value(priv->size_w, w * 1.);
	//gtk_adjustment_set_value(priv->size_h, h * 1.);
	gtk_widget_queue_draw(priv->video_area);
	gtk_widget_queue_draw(priv->preview_area);
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

void volume_changed_cb(GtkAdjustment *volume, MyMain *self) {
	GET_PRIV;
	//gdouble v = gtk_scale_button_get_value(button);
	gdouble v = gtk_adjustment_get_value(volume);
	g_object_set(priv->playbin, "volume", v, NULL);
}

void play_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->framerate_n == 0) {	//picture
		gst_element_set_state(priv->current_line,GST_STATE_NULL);
		priv->image_refresh = FALSE;
	}
	if(priv->state!=GST_STATE_PLAYING){
		gst_element_set_state(priv->current_line, GST_STATE_PLAYING);
		priv->state = GST_STATE_PLAYING;
		gtk_image_set_from_icon_name(priv->image_play,"media-playback-pause-symbolic",GTK_ICON_SIZE_BUTTON);
	}else{
		gst_element_set_state(priv->current_line, GST_STATE_PAUSED);
		priv->state = GST_STATE_PAUSED;
		gtk_image_set_from_icon_name(priv->image_play,"media-playback-start-symbolic",GTK_ICON_SIZE_BUTTON);
	}
}


gboolean progess_changed_cb(GtkScale *scale, GtkScrollType scroll,
		gdouble value, MyMain *self) {
	GET_PRIV;
	GstEvent *event;
	gdouble speed = gtk_adjustment_get_value(priv->play_speed);
	gint64 pos = value * GST_SECOND;
	gst_element_set_state(priv->play_line, GST_STATE_PLAYING);
	if (speed >= 0.) {
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				pos, GST_SEEK_TYPE_END, 0);
	} else {	//speed<0.
		event = gst_event_new_seek(speed, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
				0, GST_SEEK_TYPE_SET, pos);
	}
	gst_element_send_event(priv->play_line, event);
	gst_element_set_state(priv->play_line, priv->state);
	return TRUE;
}

void open_file_cb(GtkMenuItem *menuitem, MyMain *self) {
	gchar *uri;
	gchar *fn;
	GET_PRIV;
	GtkFileChooserDialog *dialog = gtk_file_chooser_dialog_new(
			"Open Video File", self, GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_OPEN, GTK_RESPONSE_OK, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, NULL);
	if(priv->open_file_name!=NULL){
	  gtk_file_chooser_set_filename(dialog, priv->open_file_name);
	}
	if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
		uri = gtk_file_chooser_get_uri(dialog);
		fn=gtk_file_chooser_get_filename(dialog);
		if (uri != NULL) {
			gst_element_set_state(priv->screen_line,GST_STATE_NULL);
			gst_element_set_state(priv->play_line, GST_STATE_NULL);
			g_object_set(priv->playbin, "uri", uri, NULL);
			g_free(uri);
			gst_element_set_state(priv->play_line, priv->state);
			priv->current_line=priv->play_line;
		}
		if(fn !=NULL){
		  gtk_window_set_title(self, fn);
		  g_free(priv->open_file_name);
		  priv->open_file_name=fn;
		}
	}
	gtk_widget_destroy(dialog);
	priv->image_refresh = FALSE;
}

void open_uri_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	gchar *uri;
	g_object_get(priv->playbin, "uri", &uri, NULL);
	gtk_entry_set_text(priv->open_uri, uri);
	g_free(uri);
	if (gtk_dialog_run(priv->open_uri_dialog) == GTK_RESPONSE_OK) {
		gst_element_set_state(priv->screen_line,GST_STATE_NULL);
		gst_element_set_state(priv->play_line, GST_STATE_NULL);
		uri = gtk_entry_get_text(priv->open_uri);
		g_object_set(priv->playbin, "uri", uri, NULL);
		gst_element_set_state(priv->play_line, priv->state);
		priv->current_line=priv->play_line;
		gtk_window_set_title(self, uri);
	}
	gtk_widget_hide(priv->open_uri_dialog);
	priv->image_refresh = FALSE;
}

void open_screen_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	priv->image_refresh = FALSE;
	gst_element_set_state(priv->play_line,GST_STATE_NULL);
	gst_element_set_state(priv->screen_line,priv->state);
	priv->current_line=priv->screen_line;
	gtk_window_set_title(self, "Screen Shoot");
}

void add_area_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	my_main_add_area(self, NULL, 100, 112, 100, 100);
}

void remove_area_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area != NULL) {
		if (g_hash_table_contains(priv->thread_table, priv->current_area))
			return;
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
	if (priv->current_area == NULL)
		return TRUE;
	if (priv->current_area->surf != NULL)
		cairo_surface_destroy(priv->current_area->surf);
	priv->current_area->surf = my_video_area_get_area_content(priv->video_area,
			priv->current_area->area);
	if (priv->current_area->surf == NULL)
		return TRUE;
	gdouble w, h, wr, hr;
	w = cairo_image_surface_get_width(priv->current_area->surf);
	h = cairo_image_surface_get_height(priv->current_area->surf);
	wr = alloc.width / w;
	hr = alloc.height / h;
	wr = wr < hr ? wr : hr;
	cairo_save(cr);
	cairo_translate(cr, alloc.width / 2., alloc.height / 2.);
	cairo_scale(cr, wr, wr);
	cairo_set_source_surface(cr, priv->current_area->surf, w / -2., h / -2.);
	if (wr > 1.)
		cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);//enlarge the image
	cairo_paint(cr);
	cairo_restore(cr);
	return TRUE;
}

gboolean post_tree_view_right_click_cb(GtkTreeView *widget,
		GdkEventButton *event, MyMain *self) {
	GET_PRIV;
	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_menu_popup_at_pointer(priv->post_tree_view_menu, NULL);
		return TRUE;
	}
	return FALSE;
}

void out_file_select_fn_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	gchar *name;
	GtkFileChooserDialog *dialog = gtk_file_chooser_dialog_new(
			"Select FileName", self, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_OK,
			GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_filename(dialog,
			gtk_entry_get_text(priv->out_file_filename));
	if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
		name = gtk_file_chooser_get_filename(dialog);
		gtk_entry_set_text(priv->out_file_filename, name);
		g_free(name);
	}
	gtk_widget_destroy(dialog);
}

gboolean post_bitmap_bit_dir_preview_draw_cb(GtkImage *img, cairo_t *cr,
		MyMain *self) {
	GET_PRIV;
	GString *str = g_string_new("/my/");
	ScanDirection first = gtk_combo_box_get_active(priv->post_bitmap_1st_dir);
	BitDirection dir = gtk_combo_box_get_active(priv->post_bitmap_bit_dir);
	switch (first) {
	case SCAN_DIR_BOTTOM_TO_TOP:
		g_string_append(str, "u");
		break;
	case SCAN_DIR_LEFT_TO_RIGHT:
		g_string_append(str, "r");
		break;
	case SCAN_DIR_RIGHT_TO_LEFT:
		g_string_append(str, "l");
		break;
	case SCAN_DIR_TOP_TO_BOTTOM:
	default:
		g_string_append(str, "d");
		break;
	}
	switch (dir) {
	case BIT_DIR_PARALLEL:
		g_string_append(str, "p.png");
		break;
	case BIT_DIR_VERTICAL:
	default:
		g_string_append(str, "v.png");
		break;
	}
	gtk_image_set_from_resource(img, str->str);
	g_string_free(str, TRUE);
	return FALSE;
}

gboolean post_bitmap_preview_draw_cb(GtkImage *area, cairo_t *cr, MyMain *self) {
	GET_PRIV;
	cairo_matrix_t nm;
	cairo_matrix_t *m = g_object_get_data(area, "matrix"), org, result;
	cairo_matrix_init_rotate(&nm, 0.);
	if (m == NULL) {
		m = &nm;
	};
	GtkAllocation alloc;
	gtk_widget_get_allocation(area, &alloc);
	cairo_surface_t *image = gdk_cairo_surface_create_from_pixbuf(
			gtk_image_get_pixbuf(priv->scan_preview), 1, NULL);
	gint w, h, t;
	gdouble wr, hr;
	w = cairo_image_surface_get_width(image);
	h = cairo_image_surface_get_height(image);
	wr = alloc.width / (w * 1.0);
	hr = alloc.height / (h * 1.0);
	wr = MIN(wr, hr);
	cairo_save(cr);
	if (m->xy != 0) {	//have rotate +-90,exchange w and h
		t = w;
		w = h;
		h = t;
	}
	//cairo_translate(cr,abs(alloc.width-w)/2.,abs(alloc.height-h)/2.);
	cairo_scale(cr, wr, wr);
	cairo_get_matrix(cr, &org);
	cairo_matrix_multiply(&result, m, &org);
	cairo_set_matrix(cr, &result);
	cairo_set_source_surface(cr, image, 0., 0.);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_surface_destroy(image);
	return TRUE;
}

void post_bitmap_update_scan_preview(ScanDirection first, ScanDirection second,
		MyMain *self) {
	GET_PRIV;
	cairo_matrix_t *m = calloc(1, sizeof(cairo_matrix_t));
	cairo_matrix_t a, b, *old;
	cairo_matrix_init(m, 1., 0., 0., 1., 0, 0);
	cairo_matrix_init(&a, 1., 0., 0., 1., 0, 0);
	cairo_matrix_init(&b, 1., 0., 0., 1., 0, 0);
	GdkPixbuf *img = gtk_image_get_pixbuf(priv->scan_preview);
	gint w, h;
	w = gdk_pixbuf_get_width(img);
	h = gdk_pixbuf_get_height(img);
	if (first == SCAN_DIR_RIGHT_TO_LEFT) {
		cairo_matrix_init(&a, -1., 0., 0., 1., w, 0.);	//mirror x@x=w/2
		if (second == SCAN_DIR_BOTTOM_TO_TOP) {
			cairo_matrix_init(&b, 1., 0., 0., -1., 0., h);	//mirror y@y=h/2
		}
	} else if (first == SCAN_DIR_BOTTOM_TO_TOP) {
		cairo_matrix_init_rotate(&a, -1. * G_PI_2);	//rotate 90
		cairo_matrix_translate(&a, w * -1., 0);
		if (second == SCAN_DIR_RIGHT_TO_LEFT) {
			cairo_matrix_init(&b, -1., 0., 0., 1., h, 0);//move (0,-w) and mirror x@x=h/2
		}
	} else if (first == SCAN_DIR_TOP_TO_BOTTOM) {
		cairo_matrix_init_rotate(&a, G_PI_2);	//rotate -90
		cairo_matrix_translate(&a, 0, h * -1.);
		if (second == SCAN_DIR_LEFT_TO_RIGHT) {
			cairo_matrix_init(&b, -1, 0, 0, 1, h, 0);	//mirror x@x=0
		}
	} else {
		if (second == SCAN_DIR_BOTTOM_TO_TOP) {
			cairo_matrix_init(&b, 1., 0., 0., -1., 0., 1. * h);	//mirror y@y=h/2
		}
	}
	cairo_matrix_multiply(m, &a, &b);
	old = g_object_get_data(priv->post_bitmap_preview, "matrix");
	if (old != NULL)
		g_free(old);
	g_object_set_data(priv->post_bitmap_preview, "matrix", m);
	gtk_widget_queue_draw(priv->post_bitmap_preview);
	gtk_widget_queue_draw(priv->post_bitmap_bit_dir_preview);
	return;
}

void post_bitmap_dir_changed_cb(GtkComboBoxText *combo, MyMain *self) {
	GET_PRIV;
	gint id1 = gtk_combo_box_get_active(priv->post_bitmap_1st_dir);
	gint id2 = gtk_combo_box_get_active(priv->post_bitmap_2nd_dir);
	if (combo == priv->post_bitmap_1st_dir) {
		if (id1 == SCAN_DIR_LEFT_TO_RIGHT || id1 == SCAN_DIR_RIGHT_TO_LEFT) {
			if (id2 == SCAN_DIR_LEFT_TO_RIGHT || id2 == SCAN_DIR_RIGHT_TO_LEFT)
				gtk_combo_box_set_active(priv->post_bitmap_2nd_dir,
						SCAN_DIR_TOP_TO_BOTTOM);
		} else {
			if (id2 == SCAN_DIR_TOP_TO_BOTTOM || id2 == SCAN_DIR_BOTTOM_TO_TOP)
				gtk_combo_box_set_active(priv->post_bitmap_2nd_dir,
						SCAN_DIR_LEFT_TO_RIGHT);
		}
	};
	if (combo == priv->post_bitmap_2nd_dir) {
		if (id2 == SCAN_DIR_LEFT_TO_RIGHT || id2 == SCAN_DIR_RIGHT_TO_LEFT) {
			if (id1 == SCAN_DIR_LEFT_TO_RIGHT || id1 == SCAN_DIR_RIGHT_TO_LEFT)
				gtk_combo_box_set_active(priv->post_bitmap_1st_dir,
						SCAN_DIR_TOP_TO_BOTTOM);
		} else {
			if (id1 == SCAN_DIR_TOP_TO_BOTTOM || id1 == SCAN_DIR_BOTTOM_TO_TOP)
				gtk_combo_box_set_active(priv->post_bitmap_1st_dir,
						SCAN_DIR_LEFT_TO_RIGHT);
		}
	}
	id1 = gtk_combo_box_get_active(priv->post_bitmap_1st_dir);
	id2 = gtk_combo_box_get_active(priv->post_bitmap_2nd_dir);
	post_bitmap_update_scan_preview(id1, id2, self);
}

#define set_remap_entry(_x) temp=g_strdup_printf("%.2f",remap->remap_weight._x);gtk_entry_set_text(priv->_x,temp);g_free(temp)
#define set_remap(_y) num=atof(gtk_entry_get_text(priv->_y));remap->remap_weight._y=num;
#define set_diff_entry(_x,_entry) temp=g_strdup_printf("%d",_x);gtk_entry_set_text(_entry,temp);g_free(temp)
#define set_diff(_y,_entry) _y=atoi(gtk_entry_get_text(_entry))
void post_tree_view_menu_setting_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	GdkRGBA rgba;
	GtkTreeIter iter;
	GtkTreeModel *model;
	PostCommon *post;
	PostARGBRemap *remap;
	PostBitmap *bitmap;
	PostResize *resize;
	PostBw *bw;
	PostGray *gray;
	PostDiffuse *diff;
	PostRGBFmt *fmt;
	PostTransparent *transparent;
	PostFramerate *framerate;
	OutFile *of;
	OutImgFile *oi;
	gchar *temp;
	GtkDialog *dialog = NULL;
	float num;
	GList *l, *rows = gtk_tree_selection_get_selected_rows(
			gtk_tree_view_get_selection(priv->post_tree_view), &model);
	if (rows == NULL || model == NULL)
		return;
	l = rows;
	gtk_tree_model_get_iter(model, &iter, l->data);
	gtk_tree_model_get(model, &iter, col_post, &post, -1);
	switch (post->post_type) {
	case POST_ARGB_REMAP:
		remap = post;
		dialog = priv->post_argb_remap_dialog;
		set_remap_entry(AA)
		;
		set_remap_entry(AR)
		;
		set_remap_entry(AG)
		;
		set_remap_entry(AB)
		;
		set_remap_entry(AC)
		;
		set_remap_entry(RA)
		;
		set_remap_entry(RR)
		;
		set_remap_entry(RG)
		;
		set_remap_entry(RB)
		;
		set_remap_entry(RC)
		;
		set_remap_entry(GA)
		;
		set_remap_entry(GR)
		;
		set_remap_entry(GG)
		;
		set_remap_entry(GB)
		;
		set_remap_entry(GC)
		;
		set_remap_entry(BA)
		;
		set_remap_entry(BR)
		;
		set_remap_entry(BG)
		;
		set_remap_entry(BB)
		;
		set_remap_entry(BC)
		;
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			set_remap(AA);
			set_remap(AR);
			set_remap(AG);
			set_remap(AB);
			set_remap(AC);
			set_remap(RA);
			set_remap(RR);
			set_remap(RG);
			set_remap(RB);
			set_remap(RC);
			set_remap(GA);
			set_remap(GR);
			set_remap(GG);
			set_remap(GB);
			set_remap(GC);
			set_remap(BA);
			set_remap(BR);
			set_remap(BG);
			set_remap(BB);
			set_remap(BC);
		}
		break;
	case POST_BITMAP:
		bitmap = post;
		dialog = priv->post_bitmap_dialog;
		gtk_spin_button_set_value(priv->post_bitmap_bw_thresold,
				bitmap->thresold);
		gtk_combo_box_set_active(priv->post_bitmap_mean, bitmap->mean);
		gtk_combo_box_set_active(priv->post_bitmap_1st_dir, bitmap->first);
		gtk_combo_box_set_active(priv->post_bitmap_2nd_dir, bitmap->second);
		gtk_combo_box_set_active(priv->post_bitmap_bit_dir, bitmap->bitdir);
		gtk_combo_box_set_active(priv->post_bitmap_bit_order, bitmap->order);
		gtk_combo_box_set_active(priv->post_bitmap_gray_sim, bitmap->gray);
		gtk_combo_box_set_active(priv->post_bitmap_diff_weight,bitmap->diff_radio);
		gtk_spin_button_set_value(priv->post_bitmap_gray_rank,
				bitmap->gray_rank);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			bitmap->bitdir = gtk_combo_box_get_active(
					priv->post_bitmap_bit_dir);
			bitmap->first = gtk_combo_box_get_active(priv->post_bitmap_1st_dir);
			bitmap->second = gtk_combo_box_get_active(
					priv->post_bitmap_2nd_dir);
			bitmap->gray = gtk_combo_box_get_active(priv->post_bitmap_gray_sim);
			bitmap->gray_rank = gtk_spin_button_get_value_as_int(
					priv->post_bitmap_gray_rank);
			bitmap->mean = gtk_combo_box_get_active(priv->post_bitmap_mean);
			bitmap->order = gtk_combo_box_get_active(
					priv->post_bitmap_bit_order);
			bitmap->thresold = gtk_spin_button_get_value_as_int(
					priv->post_bitmap_bw_thresold);
			bitmap->diff_radio=gtk_combo_box_get_active(priv->post_bitmap_diff_weight);
		}
		break;
	case POST_BW:
		bw = post;
		dialog = priv->post_bw_dialog;
		gtk_spin_button_set_value(priv->post_bw_thresold, bw->thresold);
		gtk_combo_box_set_active(priv->post_bw_mean, bw->mean);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			bw->thresold = gtk_spin_button_get_value_as_int(
					priv->post_bw_thresold);
			bw->mean = gtk_combo_box_get_active(priv->post_bw_mean);
		}
		break;
	case POST_DIFFUSE:
		diff = post;
		dialog = priv->post_diffuse_dialog;
		gtk_spin_button_set_value(priv->post_diffuse_rank, diff->rank);
		set_diff_entry(diff->radio.top_left,priv->post_diffuse_top_left);
		set_diff_entry(diff->radio.top_middle,priv->post_diffuse_top_middle);
		set_diff_entry(diff->radio.top_right,priv->post_diffuse_top_right);
		set_diff_entry(diff->radio.left,priv->post_diffuse_left);
		set_diff_entry(diff->radio.right,priv->post_diffuse_right);
		set_diff_entry(diff->radio.bottom_left,priv->post_diffuse_bottom_left);
		set_diff_entry(diff->radio.bottom_middle,priv->post_diffuse_bottom_middle);
		set_diff_entry(diff->radio.bottom_right,priv->post_diffuse_bottom_right);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			diff->rank = gtk_spin_button_get_value_as_int(
					priv->post_diffuse_rank);
			set_diff(diff->radio.top_left,priv->post_diffuse_top_left);
			set_diff(diff->radio.top_middle,priv->post_diffuse_top_middle);
			set_diff(diff->radio.top_right,priv->post_diffuse_top_right);
			set_diff(diff->radio.left,priv->post_diffuse_left);
			set_diff(diff->radio.right,priv->post_diffuse_right);
			set_diff(diff->radio.bottom_left,priv->post_diffuse_bottom_left);
			set_diff(diff->radio.bottom_middle,priv->post_diffuse_bottom_middle);
			set_diff(diff->radio.bottom_right,priv->post_diffuse_bottom_right);
		}
		break;
	case POST_GRAY:
		gray = post;
		dialog = priv->post_gray_dialog;
		gtk_combo_box_set_active(priv->post_gray_mean, gray->mean);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			gray->mean = gtk_combo_box_get_active(priv->post_gray_mean);
		}
		break;
	case POST_RESIZE:
		resize = post;
		dialog = priv->post_resize_dialog;
		gtk_spin_button_set_value(priv->post_resize_w, resize->resize_w);
		gtk_spin_button_set_value(priv->post_resize_h, resize->resize_h);
		gtk_toggle_button_set_active(priv->post_resize_expand, resize->expand);
		gtk_toggle_button_set_active(priv->post_resize_full, resize->full);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			resize->resize_w = gtk_spin_button_get_value_as_int(
					priv->post_resize_w);
			resize->resize_h = gtk_spin_button_get_value_as_int(
					priv->post_resize_h);
			resize->expand = gtk_toggle_button_get_active(
					priv->post_resize_expand);
			resize->full = gtk_toggle_button_get_active(priv->post_resize_full);
		}
		break;
	case POST_RGB_FMT:
		fmt = post;
		dialog = priv->post_rgb_fmt_dialog;
		gtk_combo_box_set_active(priv->post_rgb_fmt, fmt->fmt);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			fmt->fmt = gtk_combo_box_get_active(priv->post_rgb_fmt);
		}
		break;
	case POST_TRANSPARENT:
		transparent = post;
		dialog = priv->post_transparent_dialog;
		gtk_spin_button_set_value(priv->post_transparent_color_distance,
				transparent->color_distance);
		rgba.alpha = transparent->a / 255.;
		rgba.blue = transparent->b / 255.;
		rgba.green = transparent->g / 255.;
		rgba.red = transparent->r / 255.;
		gtk_color_button_set_rgba(priv->post_transparent_color, &rgba);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			transparent->color_distance = gtk_spin_button_get_value(
					priv->post_transparent_color_distance);
			gtk_color_button_get_rgba(priv->post_transparent_color, &rgba);
			transparent->a = rgba.alpha * 255;
			transparent->b = rgba.blue * 255;
			transparent->g = rgba.green * 255;
			transparent->r = rgba.red * 255;
			transparent->ia=255-transparent->a;
			transparent->ib=255-transparent->b;
			transparent->ig=255-transparent->g;
			transparent->ir=255-transparent->r;

		}
		break;
	case POST_FRAMERATE:
		framerate = post;
		dialog = priv->post_framerate_dialog;
		gtk_spin_button_set_value(priv->post_framerate_d, framerate->d);
		gtk_spin_button_set_value(priv->post_framerate_n, framerate->n);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			framerate->n = gtk_spin_button_get_value_as_int(
					priv->post_framerate_n);
			framerate->d = gtk_spin_button_get_value_as_int(
					priv->post_framerate_d);
			framerate->interval = GST_SECOND / framerate->n * framerate->d;
		}
		break;
	case OUT_FILE:
		of = post;
		dialog = priv->out_file_dialog;
		gtk_toggle_button_set_active(priv->out_file_output_head,
				of->head_output);
		gtk_toggle_button_set_active(priv->out_file_overwrite, of->over_write);
		gtk_entry_set_text(priv->out_file_filename, of->filename);
		if (of->c_source) {
			gtk_combo_box_set_active(priv->out_file_format, 1);
		} else if (of->asm_source) {
			gtk_combo_box_set_active(priv->out_file_format, 2);
		} else {
			gtk_combo_box_set_active(priv->out_file_format, 0);
		}
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			g_free(of->filename);
			of->filename = g_strdup(
					gtk_entry_get_text(priv->out_file_filename));
			of->head_output = gtk_toggle_button_get_active(
					priv->out_file_output_head);
			of->over_write = gtk_toggle_button_get_active(
					priv->out_file_overwrite);
			of->asm_source = FALSE;
			of->c_source = FALSE;
			switch (gtk_combo_box_get_active(priv->out_file_format)) {
			case 1:
				of->c_source = TRUE;
				break;
			case 2:
				of->asm_source = TRUE;
			default:
				break;
			}
		}
		break;
	case OUT_IMG_FILE:
		oi = post;
		dialog = priv->out_image_file_dialog;
		gtk_entry_set_text(priv->out_image_file_fmt, oi->name_fmt);
		gtk_file_chooser_set_action(priv->out_image_directory,
				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
		gtk_file_chooser_set_filename(priv->out_image_directory, oi->directory);
		gtk_toggle_button_set_active(priv->out_image_overwrite, oi->over_write);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			g_free(oi->directory);
			g_free(oi->name_fmt);
			oi->directory = gtk_file_chooser_get_filename(
					priv->out_image_directory);
			oi->name_fmt = g_strdup(
					gtk_entry_get_text(priv->out_image_file_fmt));
			oi->over_write = gtk_toggle_button_get_active(
					priv->out_image_overwrite);
		}
		break;
	default:
		break;
	}
	if (dialog != NULL) {
		gtk_widget_hide(dialog);
		post_view_refresh(self);
	}
	g_list_free_full(rows, gtk_tree_path_free);
	return;
}

void post_view_refresh(MyMain *self) {
	GET_PRIV;
	GtkTreeIter iter;
	GdkPixbuf *new, *old;
	GObject *obj;
	PostCommon *post;
	if (priv->current_area == NULL)
		return;
	gboolean not_empty = gtk_tree_model_get_iter_first(
			priv->current_area->process_list, &iter);
	if (!not_empty)
		return;
	cairo_surface_t *surf = my_video_area_get_area_content(priv->video_area,
			priv->current_area->area);
	if (surf == NULL)
		return;
	while (1) {
		gtk_tree_model_get(priv->current_area->process_list, &iter, col_post,
				&post, col_preview_pixbuf, &old, -1);
		new = post_preview(post, &surf);
		gtk_list_store_set(priv->current_area->process_list, &iter,
				col_preview_pixbuf, new, -1);
		cairo_surface_destroy(surf);
		if (old != NULL)
			g_object_unref(old);
		if (!gtk_tree_model_iter_next(priv->current_area->process_list,
				&iter)) {
			g_object_unref(new);
			break;
		}
		surf = gdk_cairo_surface_create_from_pixbuf(new, 1, NULL);
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
	if (gtk_toggle_button_get_active(togglebutton)) {
		full_screen(self);
	}
}

void add_post_menu_cb(GtkButton *button, MyMain *self) {
	GET_PRIV;
	gtk_menu_popup_at_widget(priv->add_post_menu, button, GDK_GRAVITY_NORTH,
			GDK_GRAVITY_NORTH, NULL);
}

void del_post_cb(GtkWidget *widget, MyMain *self) {
	GET_PRIV;
	GtkTreeIter iter;
	GtkTreePath *path;
	PostCommon *post;
	gboolean b;
	if (priv->current_area == NULL)
		return;
	if (g_hash_table_contains(priv->thread_table, priv->current_area))
		return;
	GtkTreeSelection *sel = gtk_tree_view_get_selection(priv->post_tree_view);
	GtkListStore *list_store = priv->current_area->process_list;
	GList *l, *list_row = NULL, *list = gtk_tree_selection_get_selected_rows(
			sel, &list_store);
	l = list;
	while (l != NULL) {
		list_row = g_list_append(list_row,
				gtk_tree_row_reference_new(list_store, l->data));
		l = l->next;
	}
	g_list_free_full(list, gtk_tree_path_free);

	l = list_row;
	while (l != NULL) {
		path = gtk_tree_row_reference_get_path(l->data);
		gtk_tree_model_get_iter(list_store, &iter, path);
		gtk_tree_model_get(list_store, &iter, col_post, &post, -1);
		post_free(post);
		gtk_list_store_remove(list_store, &iter);
		gtk_tree_path_free(path);
		l = l->next;
	}
	g_list_free_full(list_row, gtk_tree_row_reference_free);
}

GList* get_post_list(AreaInfo *info, GHashTable **outstream_table, MyMain *self) {
	GET_PRIV;
	GtkTreeIter iter;
	OutFile *out_file;
	PostCommon *post;
	gchar *post_name;
	guint id;
	GList *list = NULL, *area_list;
	GOutputStream *out_table = *outstream_table;
	if (gtk_tree_model_get_iter_first(info->process_list, &iter)) {
		do {
			gtk_tree_model_get(info->process_list, &iter, col_id, &id, col_name,
					&post_name, col_post, &post, -1);
			if (post->name != NULL)
				g_free(post->name);
			//update info of the post
			post->name = g_strdup_printf("%d %s %d %s", info->area->id,
					info->area->label, id, post_name);
			post->area_id = priv->current_area->area->id;
			post->framerate_d = &priv->framerate_d;
			post->framerate_n = &priv->framerate_n;
			g_free(post_name);
			list = g_list_append(list, post);
			if (post->post_type == OUT_FILE) {
				out_file = post;
				if (out_table == NULL) {
					*outstream_table = g_hash_table_new_full(g_str_hash,
							g_str_equal, NULL, NULL);
					out_table = *outstream_table;
				}
				area_list = g_hash_table_lookup(out_table, out_file->filename);
				area_list = g_list_append(area_list, info);
				g_hash_table_insert(out_table, out_file->filename, area_list);
			}
			post_run(post);
		} while (gtk_tree_model_iter_next(info->process_list, &iter));
	};
	return list;
}

gpointer run_post_thread(PostThreadData *data) {
	cairo_t *cr;
	GdkPixbuf *pixbuf;
	cairo_surface_t *post_surf;
	PostCommon *post, *next_post;
	AreaInfo *area;
	GList *al, *pl, *area_list, *post_list;
	gboolean run = TRUE, ret;
	GThread *thread = g_thread_self();
	while (run) {
		pixbuf = g_async_queue_pop(data->queue);
		if (pixbuf == thread)
			break;	//以thread*自身作为退出线程的标识
		g_mutex_lock(&data->m);
		area_list = g_hash_table_get_keys(data->areatable);
		al = area_list;
		while (al != NULL) {
			area = al->data;
			post_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					area->area->w / 1, area->area->h / 1);
			cr = cairo_create(post_surf);
			my_video_area_to_area_coordinate(cr, area->area);
			gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
			cairo_paint(cr);
			cairo_destroy(cr);
			cairo_surface_flush(post_surf);
			ret = TRUE;
			post_list = g_hash_table_lookup(data->areatable, area);
			pl = post_list;
			if (pl != NULL) {
				//初始化数据
				post = pl->data;
				post->transferdata = calloc(1, sizeof(PostTransferData));
				post->transferdata->framerate_d = *post->framerate_d;
				post->transferdata->framerate_n = *post->framerate_n;
				post->transferdata->timestamp=g_object_get_data(pixbuf,"timestamp");
			}
			while (pl != NULL) {
				post = pl->data;
				ret = post_process(post, &post_surf,
						&post->transferdata->output_data);
				if (ret == FALSE) {
					if (post->post_type == POST_FRAMERATE)
						break;	//帧率筛选，跳帧。
					g_printerr("Error:\n\tPost:%s have some problem\n",
							post->name);
				}
				pl = pl->next;
				if (pl != NULL) {
					//传递输出信息到下一个的后处理。
					next_post = pl->data;
					next_post->transferdata = post->transferdata;
				}
			}
			g_free(post->transferdata->output_data);
			g_free(post->transferdata);
			cairo_surface_destroy(post_surf);
			al = al->next;
		}
		g_list_free(area_list);
		g_object_unref(pixbuf);
		data->start = FALSE;
		g_mutex_unlock(&data->m);
	}
	g_thread_exit(data);
	return data;
}

void run_post_cb(GtkToggleButton *button, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostCommon *post;
	guint id;
	gchar *post_name;
	GList *list = NULL, *l;
	PostThreadData *data;
	GtkTreeIter iter;
	GHashTable *table = NULL;
	if (gtk_toggle_button_get_active(button) == FALSE) {
		//kill the thread
		data = g_hash_table_lookup(priv->thread_table, priv->current_area);
		if (data == NULL)
			return;
		if (g_hash_table_size(data->areatable) == 1) {
			g_async_queue_push(data->queue, data->thread);
			//wait for thread exit
			g_thread_join(data->thread);
			post_thread_data_free(data);
		} else {
			g_mutex_lock(&data->m);
			g_hash_table_remove(data->areatable, priv->current_area);
			g_mutex_unlock(&data->m);
		}
		g_hash_table_remove(priv->thread_table, priv->current_area);
		g_free(priv->current_area->area->describe);
		priv->current_area->area->describe = NULL;
		priv->refresh_preview=TRUE;
		return;
	}
	//run the thread
	if (g_hash_table_contains(priv->thread_table, priv->current_area))
		return;	//the thread have been ran.
	list = get_post_list(priv->current_area, &table, self);
	if (table != NULL)
		g_hash_table_destroy(table);
	if (list == NULL){
		//empty post process.
		gtk_toggle_button_set_active(button,FALSE);
		return;
		}
	data = post_thread_data_new();
	g_hash_table_insert(data->areatable, priv->current_area, list);
	data->thread = g_thread_new(priv->current_area->area->label,
			run_post_thread, data);
	priv->current_area->area->describe = g_strdup("Runing");
	g_hash_table_insert(priv->thread_table, priv->current_area, data);
	priv->refresh_preview=FALSE;
}

void run_post_all_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	GList *area_list = g_hash_table_get_values(priv->area_table);
	GList *post_list, *l, *sl;
	GList *sync_area_list;
	AreaInfo *area;
	GHashTable *outtable = NULL;
	GHashTable *area2postlist = g_hash_table_new_full(g_direct_hash,
			g_direct_equal, NULL, NULL);
	PostThreadData *data = NULL;
	l = area_list;
	while (l != NULL) {
		area = l->data;
		if(g_hash_table_contains(priv->thread_table,area)){
			//the post have been ran.
			l=l->next;
			continue;
		}
		post_list = get_post_list(area, &outtable, self);
		if(post_list!=NULL)g_hash_table_insert(area2postlist, area, post_list);
		l=l->next;
	}
	g_list_free(area_list);
	if (outtable != NULL) {
		sync_area_list = g_hash_table_get_values(outtable);
		sl = sync_area_list;
		while (sl != NULL) {
			area_list = sl->data;
			l = area_list;
			while (l != NULL) {
				if (data == NULL)
					data = post_thread_data_new();
				area = l->data;
				post_list = g_hash_table_lookup(area2postlist, area);
				g_hash_table_steal(area2postlist, area);
				g_hash_table_insert(data->areatable, area, post_list);
				area->area->describe = g_strdup_printf("Runing");
				g_hash_table_insert(priv->thread_table, area, data);
				l = l->next;
			}
			data->thread = g_thread_new("post thread", run_post_thread, data);
			data = NULL;
			g_list_free(area_list);
			sl = sl->next;
		}
		g_list_free(sync_area_list);
	}
	g_hash_table_destroy(outtable);
	if (g_hash_table_size(area2postlist) > 0) {
		area_list=g_hash_table_get_keys(area2postlist);
		l = area_list;
		while (l != NULL) {
			data = post_thread_data_new();
			area = l->data;
			post_list = g_hash_table_lookup(area2postlist, area);
			g_hash_table_steal(area2postlist, area);
			g_hash_table_insert(data->areatable, area, post_list);
			area->area->describe = g_strdup_printf("Runing");
			g_hash_table_insert(priv->thread_table, area, data);
			data->thread = g_thread_new("post thread", run_post_thread, data);
			l = l->next;
		}
		g_list_free(area_list);
	}
	g_hash_table_destroy(area2postlist);
}

void stop_post_all_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	AreaInfo *area;
	GList *key_list, *l;
	PostThreadData *data;
	key_list = g_hash_table_get_keys(priv->thread_table);
	l = key_list;
	while (l != NULL) {
		//kill the thread
		data = g_hash_table_lookup(priv->thread_table, l->data);
		if (data == NULL) {
			l = l->next;
			continue;
		};
		area = l->data;
		if (g_hash_table_size(data->areatable) == 1) {
			g_async_queue_push(data->queue, data->thread);
			//wait for thread exit
			g_thread_join(data->thread);
			post_thread_data_free(data);
		} else {
			g_mutex_lock(&data->m);
			g_hash_table_remove(data->areatable, area);
			g_mutex_unlock(&data->m);
		}
		g_hash_table_remove(priv->thread_table, area);
		g_free(area->area->describe);
		area->area->describe = NULL;
		l = l->next;
	}
	g_list_free(key_list);
	return;
}

void post_name_changed_cb(GtkCellRendererText *renderer, gchar *path,
		gchar *new_text, MyMain *self) {
	GET_PRIV;
	GtkTreePath *p = gtk_tree_path_new_from_string(path);
	GtkListStore *store = priv->current_area->process_list;
	GtkTreeIter iter;
	gtk_tree_model_get_iter(store, &iter, p);
	gtk_list_store_set(store, &iter, col_name, new_text, -1);
	gtk_tree_path_free(p);
}

void add_post_process(MyMain *self, PostCommon *post, gchar *name,
		PostType type) {
	GET_PRIV;
	GtkTreeIter iter;
	GtkTreePath *path;
	AreaInfo *info = priv->current_area;
	post->post_type = type;
	post->widget_draw_queue = priv->widget_draw_queue;
	post->duration = &priv->duration;
	guint id = gtk_tree_model_iter_n_children(info->process_list, NULL);
	gchar *n = g_strdup_printf("%s %d", name, id);
	GList *list=gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(priv->post_tree_view),NULL);
	if(list!=NULL){
		path=list->data;
		gtk_tree_model_get_iter(info->process_list,&iter,path);
		g_list_free_full(list,gtk_tree_path_free);
		gtk_list_store_insert_after( info->process_list,&iter,&iter);
	}else{
		gtk_list_store_append(info->process_list, &iter);
	}
	GtkImage *ti = NULL;
	GdkPixbuf *pixbuf = NULL;
	switch (type) {
	case POST_ARGB_REMAP:
		ti = priv->remap;
		break;
	case POST_BITMAP:
		ti = priv->scan;
		break;
	case POST_BW:
		ti = priv->bw;
		break;
	case POST_DIFFUSE:
		ti = priv->diff;
		break;
	case POST_GRAY:
		ti = priv->gray;
		break;
	case POST_RESIZE:
		ti = priv->resize;
		break;
	case POST_RGB_FMT:
		ti = priv->rgbfmt;
		break;
	case POST_FRAMERATE:
		ti = priv->framerate;
		break;
	case POST_TRANSPARENT:
		ti = priv->transparent;
		break;
	case OUT_FILE:
		ti = priv->file;
		break;
	case OUT_IMG_FILE:
		ti = priv->image_file;
		break;
	case OUT_WINDOWS:
		ti = priv->window;
		break;
	default:
		ti = NULL;
		break;
	}
	if (ti != NULL)
		pixbuf = gtk_image_get_pixbuf(ti);
	gtk_list_store_set(info->process_list, &iter, col_post, post, col_id, id,
			col_name, n, col_preview_pixbuf, NULL, col_type_pixbuf, pixbuf,
			col_type, type, -1);
	g_free(n);
	//post_view_refresh(self);
}

void add_to_gray_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostGray *post = g_malloc0(sizeof(PostGray));
	add_post_process(self, post, "Gray", POST_GRAY);
}

void add_to_bw_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostBw *post = g_malloc0(sizeof(PostBw));
	add_post_process(self, post, "Black White", POST_BW);
	post->thresold = 127;

}

void add_argb_remap_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostARGBRemap *post = g_malloc0(sizeof(PostARGBRemap));
	add_post_process(self, post, "ARGB Remap", POST_ARGB_REMAP);
	post->remap_weight.AA = 1;
	post->remap_weight.RR = 1;
	post->remap_weight.GG = 1;
	post->remap_weight.BB = 1;
}

void add_error_diffuse_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostDiffuse *post = g_malloc0(sizeof(PostDiffuse));
	post->rank = 1;
	post->radio.bottom_middle = diff_332.bottom_middle;
	post->radio.right = diff_332.right;
	post->radio.bottom_right = diff_332.bottom_right;
	add_post_process(self, post, "Error Diffusion", POST_DIFFUSE);
}

void add_transparent_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostTransparent *post = g_malloc0(sizeof(PostTransparent));
	add_post_process(self, post, "Transparent", POST_TRANSPARENT);
	post->a = 255;
	post->ia=0;
	post->ib=255;
	post->ig=255;
	post->ir=255;
	post->color_distance = 10.;
}

void add_rgb_data_format_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostRGBFmt *post = g_malloc0(sizeof(PostRGBFmt));
	add_post_process(self, post, "RGB Format", POST_RGB_FMT);
	post->fmt = RGB_FORMAT_888;
}

void add_scan_bitmap_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostBitmap *post = g_malloc0(sizeof(PostBitmap));
	add_post_process(self, post, "Scan Bitmap", POST_BITMAP);
	post->thresold = 127;
	post->gray_rank = 1;
	post->gray = GRAY_SIM_DIFFUSE;
	post->first = SCAN_DIR_LEFT_TO_RIGHT;
	post->second = SCAN_DIR_TOP_TO_BOTTOM;
}

void add_resize_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostResize *post = g_malloc0(sizeof(PostResize));
	add_post_process(self, post, "Resize", POST_RESIZE);
	post->resize_w = 128;
	post->resize_h = 64;
	post->full = TRUE;
}

void add_framerate_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostFramerate *post = g_malloc0(sizeof(PostFramerate));
	add_post_process(self, post, "Framerate Limit", POST_FRAMERATE);
	post->d = 1;
	post->n = 25;
	post->interval = GST_SECOND / post->n * post->d;
}

void add_output_window_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	PostCommon *post = g_malloc0(sizeof(OutWindow));
	add_post_process(self, post, "Window Output", OUT_WINDOWS);
}

void add_output_raw_file_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	OutFile *post = g_malloc0(sizeof(OutFile));
	add_post_process(self, post, "File Output", OUT_FILE);
	post->filename = g_strdup("output.txt");
}

void add_output_image_file_cb(GtkMenuItem *menuitem, MyMain *self) {
	GET_PRIV;
	if (priv->current_area == NULL)
		return;
	OutImgFile *post = g_malloc0(sizeof(OutImgFile));
	add_post_process(self, post, "Image Output", OUT_IMG_FILE);
	post->name_fmt = g_strdup("image-%d.png");
	post->directory = g_strdup(
			g_get_user_special_dir(G_USER_DIRECTORY_PICTURES));
}

gboolean message_watch_cb(GstBus *bus, GstMessage *message, MyMain *self) {
	GET_PRIV;
	gint64 pos, dur;
	GstQuery *query;
	GstCaps *cap;
	GstPad *pad;
	GstStructure *structure;
	GList *threads = g_hash_table_get_values(priv->thread_table);
	GList *l;
	PostThreadData *data;
	GtkWidget *widget;
	GstElement *sink;
	if (message->type == GST_MESSAGE_ELEMENT) {
		GstStructure *s = gst_message_get_structure(message);
		if (gst_structure_has_field_typed(s, "pixbuf", GDK_TYPE_PIXBUF)) {
			//update the time info
			gst_element_query_position(priv->current_line, GST_FORMAT_TIME, &pos);
			gst_element_query_duration(priv->current_line, GST_FORMAT_TIME, &dur);
			gtk_adjustment_set_upper(priv->progress, dur / GST_SECOND);
			gtk_adjustment_set_value(priv->progress, pos / GST_SECOND);
			priv->position = pos;
			priv->duration = dur;
			//push the pixbuf to the thread
			GdkPixbuf *p = NULL;
			gst_structure_get(s, "pixbuf", GDK_TYPE_PIXBUF, &p, NULL);
			g_object_set_data(p,"timestamp",pos);
			my_video_area_set_pixbuf(priv->video_area, p);
			gtk_widget_queue_draw(priv->video_area);
			gtk_widget_queue_draw(priv->preview_area);

			//stop the runing area preview to reduce resource
			if(priv->refresh_preview)post_view_refresh(self);

			//push the pixbuf to the runing post thread
			l = threads;
			if (l != NULL && priv->image_refresh) {
				while (l != NULL) {
					data = l->data;
					g_async_queue_push(data->queue, g_object_ref(p));
					l = l->next;
				}
				g_list_free(threads);
			}
			priv->image_refresh = TRUE;
			g_object_unref(p);
			//redraw the widget that post thread indicate,since GTK is not MT SAFT.
			while (1) {
				widget = g_async_queue_try_pop(priv->widget_draw_queue);
				if (widget == NULL)
					break;
				gtk_widget_queue_draw(widget);
			}


			if(priv->current_line==priv->play_line){
				sink=priv->tee_sink;
			}else{
				//screen line
				sink=gst_bin_get_by_name(priv->current_line,"sink");
			}
			pad = gst_element_get_static_pad(sink, "sink");
			cap = gst_pad_get_current_caps(pad);
			structure = gst_caps_get_structure(cap, 0);
			gst_structure_get(structure, "framerate", GST_TYPE_FRACTION,
					&priv->framerate_n, &priv->framerate_d, NULL);
			gst_caps_unref(cap);
			gst_object_unref(pad);
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
	gtk_widget_class_set_template_from_resource(klass, "/my/MyMain.glade");
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
	gtk_widget_class_bind_template_child_private(klass,MyMain,image_play);
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
	gtk_widget_class_bind_template_child_private(klass, MyMain, framerate);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_tree_view_menu);
	gtk_widget_class_bind_template_child_private(klass, MyMain, scan_preview);
	gtk_widget_class_bind_template_child_private(klass, MyMain, run_post);
	gtk_widget_class_bind_template_child_private(klass, MyMain, del_post);
	gtk_widget_class_bind_template_child_private(klass, MyMain, remove);
	gtk_widget_class_bind_template_child_private(klass, MyMain, fit_size);

	//post setting dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain, post_bw_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_gray_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_resize_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_rgb_fmt_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_argb_remap_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_transparent_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_framerate_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_file_dialog);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_image_file_dialog);

	//post_argb_remap_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain, AA);
	gtk_widget_class_bind_template_child_private(klass, MyMain, AR);
	gtk_widget_class_bind_template_child_private(klass, MyMain, AG);
	gtk_widget_class_bind_template_child_private(klass, MyMain, AB);
	gtk_widget_class_bind_template_child_private(klass, MyMain, AC);
	gtk_widget_class_bind_template_child_private(klass, MyMain, RA);
	gtk_widget_class_bind_template_child_private(klass, MyMain, RR);
	gtk_widget_class_bind_template_child_private(klass, MyMain, RG);
	gtk_widget_class_bind_template_child_private(klass, MyMain, RB);
	gtk_widget_class_bind_template_child_private(klass, MyMain, RC);
	gtk_widget_class_bind_template_child_private(klass, MyMain, GA);
	gtk_widget_class_bind_template_child_private(klass, MyMain, GR);
	gtk_widget_class_bind_template_child_private(klass, MyMain, GG);
	gtk_widget_class_bind_template_child_private(klass, MyMain, GB);
	gtk_widget_class_bind_template_child_private(klass, MyMain, GC);
	gtk_widget_class_bind_template_child_private(klass, MyMain, BA);
	gtk_widget_class_bind_template_child_private(klass, MyMain, BR);
	gtk_widget_class_bind_template_child_private(klass, MyMain, BG);
	gtk_widget_class_bind_template_child_private(klass, MyMain, BB);
	gtk_widget_class_bind_template_child_private(klass, MyMain, BC);

	//post_bitmap_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_bw_thresold);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_mean);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_1st_dir);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_2nd_dir);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_bit_dir);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_bit_order);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_gray_sim);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_gray_rank);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_preview);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_bit_dir_preview);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bitmap_diff_weight);

	//post_bw_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_bw_thresold);
	gtk_widget_class_bind_template_child_private(klass, MyMain, post_bw_mean);

	//post_diffuse_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_rank);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_top_left);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_top_middle);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_top_right);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_left);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_right);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_bottom_left);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_bottom_middle);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_diffuse_bottom_right);

	//post_gray_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain, post_gray_mean);

	//post_resize_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain, post_resize_w);
	gtk_widget_class_bind_template_child_private(klass, MyMain, post_resize_h);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_resize_expand);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_resize_full);

	//post_rgb_fmt_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain, post_rgb_fmt);

	//post_transparent_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_transparent_color_distance);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_transparent_color);

	//post_framerate_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_framerate_n);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			post_framerate_d);

	//out_image_file_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_image_file_fmt);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_image_directory);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_image_overwrite);

	//out_file_dialog
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_file_filename);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_file_format);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_file_output_head);
	gtk_widget_class_bind_template_child_private(klass, MyMain,
			out_file_overwrite);

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
	gtk_widget_class_bind_template_callback(klass,
			post_tree_view_right_click_cb);
	gtk_widget_class_bind_template_callback(klass,
			post_tree_view_menu_setting_cb);
	gtk_widget_class_bind_template_callback(klass, out_file_select_fn_cb);
	gtk_widget_class_bind_template_callback(klass, post_bitmap_dir_changed_cb);
	gtk_widget_class_bind_template_callback(klass, post_bitmap_preview_draw_cb);
	gtk_widget_class_bind_template_callback(klass,
			post_bitmap_bit_dir_preview_draw_cb);
	gtk_widget_class_bind_template_callback(klass, run_post_all_cb);
	gtk_widget_class_bind_template_callback(klass, stop_post_all_cb);

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
	gtk_widget_class_bind_template_callback(klass, add_framerate_cb);
}


// dx9screencapsrc ! autovideoconvert ! queue ! autovideosink

static void my_main_init(MyMain *self) {
	gint w, h;
	gtk_widget_init_template(self);
	GET_PRIV;
	priv->video_area = my_video_area_new();
	priv->current_area = NULL;
	priv->thread_table = g_hash_table_new(g_direct_hash, g_direct_equal);
	priv->area_table = g_hash_table_new(g_direct_hash, g_direct_equal);
	priv->widget_draw_queue = g_async_queue_new();
	priv->open_file_name=NULL;
	priv->refresh_preview=TRUE;
	gtk_container_add(priv->video_box, priv->video_area);
	my_video_area_set_pixbuf(priv->video_area,
			gdk_pixbuf_new_from_file("dog.jpg", NULL));
	gtk_paned_set_position(priv->paned, 550);
	g_object_bind_property(priv->run_post, "active", priv->del_post,
			"sensitive", G_BINDING_INVERT_BOOLEAN | G_BINDING_SYNC_CREATE);
	g_object_bind_property(priv->run_post, "active", priv->remove, "sensitive",
			G_BINDING_INVERT_BOOLEAN | G_BINDING_SYNC_CREATE);

	g_signal_connect(priv->video_area, "area_select", area_select, self);
	g_signal_connect(priv->video_area, "area_move", area_move, self);
	g_signal_connect(priv->video_area, "area_resize", area_resize, self);
	g_signal_connect(priv->video_area, "area_rotate", area_rotate, self);

	GstBin *bin = gst_bin_new("tee");
	priv->tee_sink = bin;
	GstElement *tee = gst_element_factory_make("tee", "tee");
	gst_bin_add(bin, tee);
	GstPad *src_pad, *sink_pad;
	sink_pad = gst_element_get_static_pad(tee, "sink");
	gst_element_add_pad(bin, gst_ghost_pad_new("sink", sink_pad));
	gst_object_unref(sink_pad);

	GstElement *queue = gst_element_factory_make("queue", "queue");
	gst_bin_add(bin, queue);
	sink_pad = gst_element_get_static_pad(queue, "sink");
	src_pad = gst_element_get_request_pad(tee, "src_0");
	gst_pad_link(src_pad, sink_pad);
	gst_object_unref(src_pad);
	gst_object_unref(sink_pad);

	priv->video_sink = gst_element_factory_make("gdkpixbufsink", "sink");
	gst_bin_add(bin, priv->video_sink);
	src_pad = gst_element_get_static_pad(queue, "src");
	sink_pad = gst_element_get_static_pad(priv->video_sink, "sink");
	gst_pad_link(src_pad, sink_pad);
	gst_object_unref(src_pad);
	gst_object_unref(sink_pad);

	priv->playbin = gst_element_factory_make("playbin", "playbin");
	priv->play_line = gst_pipeline_new("line");
	gst_bin_add_many(priv->play_line, priv->playbin, NULL);
	//g_object_set(priv->playbin,"video-sink",priv->video_sink,"uri","file:///home/tom/eclipse-workspace/Spring.mp4",NULL);
	g_object_set(priv->playbin, "video-sink", priv->tee_sink, "uri",
			"file:///home/tom/dwhelper/2020VSINGERLIVE%E6%BC%94%E5%94%B1%E4%BC%9A%E8%AF%A6%E6%83%85%E4%BB%8B%E7%BB%8D-2020VSINGERLIVE%E6%BC%94%E5%94%B1%E4%BC%9A%E5%9C%A8%E7%BA%BF%E8%A7%82%E7%9C%8B-2020VSINGERLIV.mp4",
			"volume", gtk_adjustment_get_value(priv->volume),
			NULL);
	gst_bus_add_watch(gst_pipeline_get_bus(priv->play_line), message_watch_cb,
			self);

#ifdef G_OS_WIN32
	priv->screen_line=gst_parse_launch("dx9screencapsrc name=src ! autovideoconvert ! queue name=queue ! gdkpixbufsink name=sink", NULL);
#else
	priv->screen_line=gst_parse_launch("ximagesrc name=src ! autovideoconvert ! queue name=queue ! gdkpixbufsink name=sink", NULL);
#endif
	queue=gst_bin_get_by_name(priv->screen_line,"queue");
	gst_bus_add_watch(gst_pipeline_get_bus(priv->screen_line), message_watch_cb,
			self);
	gst_element_set_state(priv->screen_line,GST_STATE_NULL);
	gst_element_set_state(priv->play_line, GST_STATE_PLAYING);
	priv->state = GST_STATE_PLAYING;
	priv->current_line=priv->play_line;
	my_main_add_area(self, "test", 100, 200, 100, 200);
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
		if (gtk_tree_model_get_iter_first(liststore, &iter)) {
			do {
				gtk_tree_model_get(liststore, &iter, col_preview_pixbuf,
						&preview, col_post, &post, -1);
				gtk_list_store_set(liststore, &iter, col_preview_pixbuf, NULL,
						col_post, NULL, -1);
				if (post != NULL)
					post_free(post);
				if (preview != NULL)
					g_object_unref(preview);
			} while (gtk_tree_model_iter_next(liststore, &iter));
		}
		g_object_unref(info->process_list);
		g_free(info);
	}
}
