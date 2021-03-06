/*
 * PostProcess.c
 *
 *  Created on: 2021年7月29日
 *      Author: tom
 */

#define	F_OK	0	/* Check for file existence */
#define	X_OK	1	/* Check for execute permission. */
#define	W_OK	2	/* Check for write permission */
#define	R_OK	4	/* Check for read permission */

#include "PostProcess.h"
#include "string_helper.h"

PostProcessFunc post_get_process_func (PostCommon *post) {
  switch (post->post_type) {
    case POST_BW:
      return post_bw;
      break;
    case POST_BITMAP:
      return post_bitmap;
      break;
    case POST_DIFFUSE:
      return post_diffuse;
      break;
    case POST_GRAY:
      return post_gray;
      break;
    case POST_RGB_FMT:
      return post_rgb_fmt;
      break;
    case POST_ARGB_REMAP:
      return post_argb_remap;
      break;
    case POST_TRANSPARENT:
      return post_transparent;
      break;
    case POST_RESIZE:
      return post_resize;
      break;
    case POST_FRAMERATE:
      return post_framerate;
      break;
    case OUT_FILE:
      return out_file;
      break;
    case OUT_IMG_FILE:
      return out_img_file;
      break;
    case OUT_WINDOWS:
      return out_windows;
      break;
    default:
      break;
  }
  return NULL;
}

gboolean post_process (PostCommon *post, guint8 **data, gpointer *out) {
  gboolean ret = FALSE;
  PostProcessFunc func = post_get_process_func (post);
  if (func != NULL) ret = func (post, data, out);
  return ret;
}

void destroy_surf (guchar *pixels, cairo_surface_t *out) {
  cairo_surface_destroy (out);
}

GdkPixbuf* post_preview (PostCommon *post, cairo_surface_t **s) {
  cairo_surface_t *surf = *s;
  if (surf == NULL) return NULL;
  guint w, h, i;
  guint8 rank = 0;
  PostRGBFmt *fmt;
  PostGray *gray;
  PostDiffuse *diff;
  PostARGBRemap *remap;
  PostBitmap *bit;
  PostResize *resize;
  OutWindow *window;
  gdouble rw, rh;
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);
  cairo_surface_t *out, *temp;
  if (post->post_type == POST_RESIZE) {
    resize = post;
    out = NULL;
  }
  else {
    out = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  }
  cairo_t *cr = cairo_create (out);
  if (post->post_type == POST_RESIZE) {
    //resize the content and then paint to the surface.
    post_resize (resize, &surf, NULL);
    out = cairo_surface_reference (surf);
    *s = out;
    //update w,h for create pixbuf.
    w = resize->resize_w;
    h = resize->resize_h;
  }
  else if (post->post_type == POST_DIFFUSE) {
    //do nothing
  }
  else {
    //copy the content to the surface.
    cairo_set_source_surface (cr, surf, 0, 0);
    cairo_paint (cr);
  }
  cairo_surface_flush (out);
  guint8 t, *p, *data = cairo_image_surface_get_data (out);
  //modify the content by the post.
  //be aware of the w=resize->resize_w , h=resize_h when post->post_type==POST_RESIZE;
  switch (post->post_type) {
    case POST_BITMAP:
      bit = post;
      if (bit->gray == GRAY_SIM_DIFFUSE) {
	cairo_rectangle (cr, 0, 0, w, h);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_fill (cr);
	surf_rgba_to_gray_color (surf, bit->mean);
	switch(bit->diff_radio){
	case 1:
	img_error_diffusion (cairo_image_surface_get_data (surf), data, w, h, 4,
			     bit->gray_rank, &diff_332);break;
	default:
	img_error_diffusion (cairo_image_surface_get_data (surf), data, w, h, 4,
				     bit->gray_rank, &diff_332UP);break;
    }
	surf_rgba_to_bw_color (out, MEAN_NUM, bit->thresold);

      }else if (bit->gray == GRAY_SIM_MUL_THRESOLD) {
	surf_rgba_to_gray_color (out, bit->mean);
	img_rank (cairo_image_surface_get_data (out),
		  cairo_image_surface_get_data (out), w * 4, h, bit->gray_rank);
      }
      else {
	surf_rgba_to_bw_color (out, MEAN_NUM, bit->thresold);
      }
      break;
    case POST_ARGB_REMAP:
      remap = post;
      img_argb_remap (data, data, w, h, &remap->remap_weight);
      break;
    case POST_DIFFUSE:
      diff = post;
      img_error_diffusion (cairo_image_surface_get_data (surf), data, w, h, 4,
			   diff->rank, &diff->radio);
      break;
    case POST_GRAY:
      gray = post;
      surf_rgba_to_gray_color (out, gray->mean);
      break;
    case POST_RGB_FMT:
      fmt = post;
      switch (fmt->fmt) {
RGB_FORMAT_444: rank = 16;
	break;
RGB_FORMAT_565: rank = 32;
	break;
RGB_FORMAT_666: rank = 64;
	break;
RGB_FORMAT_888: default:
	rank = 255;
      }
      img_rank (data, data, w * 4, h, rank);
      break;
    case POST_TRANSPARENT:
      post_transparent (post, &out, NULL);
      break;
    case POST_BW:
      post_bw (post, &out, NULL);
      break;
    case OUT_WINDOWS:
      window = post;
//		if (window->display_widget == NULL)
//			post_run(window);
//		if(out_windows(window, &surf, NULL))gtk_widget_queue_draw(window->display_widget);
      break;
    default:
      break;
  }
  cairo_surface_mark_dirty (out);
  cairo_destroy (cr);
  p = data;
  //GdkPixbuf            p[0] = red; p[1] = green; p[2] = blue; p[3] = alpha;
  //CAIRO_FORMAT_ARGB32  d[0] = blue;d[1] = green; d[2] = red;  d[3] = alpha;
  for (i = 0; i < w * h; i++) {
    //convert format from CAIRO_FORMAT_ARGB32 to GdkPixbuf
    t = p[0];    //t=blue
    p[0] = p[2]; //p[0]=red
    p[2] = t;    //p[2]=blue
    p += 4;      //next pixel
  }
  //be aware of the w=resize->resize_w , h=resize_h when post->post_type==POST_RESIZE;
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB, TRUE,
						8, w, h, w * 4, destroy_surf,
						out);
  return pixbuf;
}

void post_run (PostCommon *post) {
  PostBitmap *bitmap;
  PostFramerate *framerate;
  OutImgFile *image;
  OutFile *file;
  OutWindow *win;
  GFile *f;
  switch (post->post_type) {
    case POST_BITMAP:
      bitmap = post;
      bitmap->rank_index = 0;
      bitmap->rank_dir = 1;
      break;
    case POST_FRAMERATE:
      framerate = post;
      framerate->previous_pos = 0;
      break;
    case OUT_IMG_FILE:
      image = post;
      image->index = 0;
      break;
    case OUT_WINDOWS:
      win = post;
      if (win->display_widget == NULL) create_display_widget (win);
      break;
    case OUT_FILE:
      file = post;
      file->index = 0;
      f = g_file_new_for_path (file->filename);
      if (f == NULL) {
	file->out = NULL;
	return;
      }
      if (file->over_write) unlink (file->filename);
      file->out = g_file_append_to (f, G_FILE_CREATE_NONE, NULL, NULL);
      g_object_unref (f);
      break;
    default:
      break;
  }
}

void post_stop (PostCommon *post) {
  OutFile *file;
  OutWindow *win;
  GtkWindow *w;
  switch (post->post_type) {
    case OUT_WINDOWS:
      win = post;
      while (g_async_queue_remove (win->com.widget_draw_queue,
				   win->display_widget));
      if (win->display_widget != NULL) {
	w = gtk_widget_get_toplevel (win->display_widget);
	gtk_widget_destroy (w);
	win->display_widget = NULL;
      }
      break;
    case OUT_FILE:
      file = post;
      if (file->out != NULL) {
	g_output_stream_close (file->out, NULL, NULL);
	g_object_unref (file->out);
	file->out = NULL;
      }
      break;
    default:
      break;
  }
}

void post_free (PostCommon *post) {
  OutFile *file;
  OutImgFile *img;
  OutWindow *win;
  if (post == NULL) return;
  g_free (post->name);
  switch (post->post_type) {
    case OUT_FILE:
      post_stop (post);
      file = post;
      g_free (file->filename);
      g_free (file);
      break;
    case OUT_IMG_FILE:
      img = post;
      g_free (img->name_fmt);
      g_free (img->directory);
      g_free (img);
      break;
    case OUT_WINDOWS:
      win = post;
      g_object_set_data (win->display_widget, "deleted", GUINT_TO_POINTER(1)); //通知显示窗原后处理器已删除。
      g_free (win);
      break;
    default:
      g_free (post);
      break;
  }
  return;
}

gboolean post_bw (PostBw *bw, cairo_surface_t **s, gpointer *out) {
  PostCommon *com = &bw->com;
  cairo_surface_t *surf = *s;
  cairo_surface_flush (surf);
  surf_rgba_to_bw_color (surf, bw->mean, bw->thresold);
  return TRUE;
}

gboolean post_gray (PostGray *gray, cairo_surface_t **s, gpointer *out) {
  PostCommon *com = &gray->com;
  cairo_surface_t *surf = *s;
  cairo_surface_flush (surf);
  surf_rgba_to_gray_color (surf, gray->mean);
  return TRUE;
}

gboolean post_rgb_fmt (PostRGBFmt *rgb_fmt, cairo_surface_t **s, gpointer *out) {
  PostCommon *com = &rgb_fmt->com;
  cairo_surface_t *surf = *s;
  guint32 w, h, *data = cairo_image_surface_get_data (surf);
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);
  if (*out != NULL) g_free (*out);
  PostTransferData *t = com->transferdata;
  t->output_size = img_rgb_format (data, out, w, h, rgb_fmt->fmt);
  t->w = w;
  t->h = h;
  return TRUE;
}

gboolean post_argb_remap (PostARGBRemap *argbremap, cairo_surface_t **s,
			  gpointer *out) {
  PostCommon *com = &argbremap->com;
  cairo_surface_t *surf = *s;
  guint8 *data = cairo_image_surface_get_data (surf);
  guint32 w, h;
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);
  img_argb_remap (data, data, w, h, &argbremap->remap_weight);
  cairo_surface_mark_dirty (surf);
  return TRUE;
}

gboolean post_diffuse (PostDiffuse *diffuse, cairo_surface_t **s, gpointer *out) {
  PostCommon *com = &diffuse->com;
  cairo_surface_t *surf = *s;

  guint8 *data = cairo_image_surface_get_data (surf);

  guint32 w, h;
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);
  cairo_surface_t *osurf = cairo_image_surface_create (
      cairo_image_surface_get_format (surf), w, h);
  guint8 *odata = cairo_image_surface_get_data (osurf);
  guint8 pixal_size = cairo_image_surface_get_stride (surf) / w;
  img_error_diffusion (data, odata, w, h, pixal_size, diffuse->rank,
		       &diffuse->radio);
  cairo_surface_mark_dirty (osurf);
  cairo_surface_destroy (surf);
  *s = osurf;
  return TRUE;
}

gboolean post_transparent (PostTransparent *transparent, cairo_surface_t **s,
			   gpointer *out) {
  PostCommon *com = &transparent->com;
  cairo_surface_t *surf = *s;
  gdouble d = 0, dr, dg, db, da;
  guint32 size, i;
  guint8 *p = cairo_image_surface_get_data (surf);
  guint32 w, h;
  guint temp;
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);
  size = w * h;
  for (i = 0; i < size; i++) {
    dr = (p[2] - transparent->r);
    dg = (p[1] - transparent->g);
    db = (p[0] - transparent->b);
    da = (p[3] - transparent->a);
    d = sqrt (dr * dr + dg * dg + db * db + da * da);
    if (d <= transparent->color_distance) {
      d = d / transparent->color_distance;
      p[0] = p[0] - transparent->b>0 ? p[0] - transparent->b : 0;
      p[1] = p[1] - transparent->g>0 ? p[1] - transparent->g : 0;
      p[2] = p[2] - transparent->r>0 ? p[2] - transparent->r : 0;
      p[3] = p[3] - transparent->a>0 ? p[3] - transparent->a : 0;
//      temp=p[0] + transparent->ib;
//      p[0] = temp>0xff?0xff:temp;
//      p[0] *= d;
//      temp=p[1] + transparent->ig;
//      p[1] = temp>0xff?0xff:temp;
//      p[1] *= d;
//      temp=p[2] + transparent->ir;
//      p[2] = temp>0xff?0xff:temp;
//      p[2] *= d;
      p[3] =  sin(d*G_PI_2)*0xff ;
    }
    p += 4;
  }
  cairo_surface_mark_dirty (surf);
  return TRUE;
}

gboolean post_bitmap (PostBitmap *bitmap, cairo_surface_t **s, gpointer *out) {
  PostCommon *com = &bitmap->com;
  guint8 thresold = bitmap->thresold;
  cairo_surface_t *surf = *s;
  cairo_surface_t *argb = NULL, *a1 = NULL, *temp;
  guint32 w, h;
  cairo_t *cr;
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);

  argb = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  temp = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  switch (bitmap->gray) {
    case GRAY_SIM_DIFFUSE:
      memcpy (cairo_image_surface_get_data (temp),
	      cairo_image_surface_get_data (surf), w * h * 4);
      surf_rgba_to_gray_color (temp, bitmap->mean);
      cairo_surface_mark_dirty(temp);
      switch(bitmap->diff_radio){
      case 1:
      img_error_diffusion (cairo_image_surface_get_data (temp),
    			   cairo_image_surface_get_data (argb), w, h, 4,
    			   bitmap->gray_rank, &diff_332UP);break;
      default:
      img_error_diffusion (cairo_image_surface_get_data (temp),
			   cairo_image_surface_get_data (argb), w, h, 4,
			   bitmap->gray_rank, &diff_332UP);break;
      }
      surf_rgba_to_bw_color (argb, MEAN_NUM, bitmap->thresold);
      break;
    case GRAY_SIM_MUL_THRESOLD:
      memcpy (cairo_image_surface_get_data (argb),
	      cairo_image_surface_get_data (surf), w * h * 4);
      thresold = (255 - bitmap->thresold) / bitmap->gray_rank;
      thresold = bitmap->thresold + thresold * bitmap->rank_index;
      bitmap->rank_index += bitmap->rank_dir;
      if (bitmap->rank_index > bitmap->gray_rank || bitmap->rank_index == 0)
	bitmap->rank_dir = bitmap->rank_dir > 0 ? -1 : 1;
      surf_rgba_to_gray_color (argb, bitmap->mean);
      img_rank (cairo_image_surface_get_data (argb),
		cairo_image_surface_get_data (argb), w * 4, h,
		bitmap->gray_rank);
      surf_rgba_to_bw_color (argb, MEAN_NUM, thresold);
      break;
    default:
      surf_rgba_to_bw_color (argb, MEAN_NUM, bitmap->thresold);
      break;
  }
  cairo_surface_mark_dirty(argb);
  a1 = cairo_image_surface_create (CAIRO_FORMAT_A1, w, h);
  uint32_t i, j=0;
  uint8_t k;
  uint32_t *bit = cairo_image_surface_get_data (a1);
  uint8_t *src = cairo_image_surface_get_data (argb);
  uint32_t _w = ceil (w / 32.); //width length should alien 32bit
  for (i = 0; i < _w * h; i++) {
    *bit = 0;
    for (k = 0; k < 32; k++) {
      *bit = *bit >> 1;
      if (*src > 127) *bit |= 0x80000000;
      src += 4;
      j++;
      if (j >= w) {
	j = 0;
	break;
      }
    }
    bit++;
  }
  cairo_surface_mark_dirty(a1);
  if (*out != NULL) g_free (*out);
  PostTransferData *t = com->transferdata;
  t->output_size = surf_a1_transform_by_scan (a1, out, bitmap->first,
					      bitmap->second, bitmap->order,
					      bitmap->bitdir);
  t->w = w;
  t->h = h;
  cairo_surface_destroy (a1);
  cairo_surface_destroy (argb);
  cairo_surface_destroy (temp);
  return TRUE;
}

gboolean post_resize (PostResize *resize, cairo_surface_t **s, gpointer *out) {
  gdouble wr, hr;
  guint32 w, h;
  cairo_surface_t *surf = *s;
  if (surf == NULL) return FALSE;
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);
  wr = resize->resize_w / (w * 1.);
  hr = resize->resize_h / (h * 1.);
  if (!resize->expand) {
    if (resize->full) {
      wr = wr > hr ? wr : hr;
      hr = wr;
    }
    else {
      wr = wr < hr ? wr : hr;
      hr = wr;
    }
  }
  cairo_surface_t *osurf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						       resize->resize_w,
						       resize->resize_h);
  cairo_t *cr = cairo_create (osurf);
  cairo_translate (cr, resize->resize_w / 2., resize->resize_h / 2.);
  cairo_scale (cr, wr, hr);
  cairo_set_source_surface (cr, surf, w / -2., h / -2.);
  if (wr > 1. || hr > 1.)
    cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (surf);
  *s = osurf;
  return TRUE;
}

gboolean post_framerate (PostFramerate *framerate, cairo_surface_t **s,
			 gpointer *out) {
  guint64 interval = ABS(
      framerate->com.transferdata->timestamp - framerate->previous_pos);
//  g_print ("%ld > %ld = %s\n", interval, framerate->interval,
//	   interval > framerate->interval ? "True" : "False");
  if (interval > framerate->interval) {
    framerate->com.transferdata->framerate_d = framerate->d;
    framerate->com.transferdata->framerate_n = framerate->n;
    framerate->previous_pos = framerate->com.transferdata->timestamp;
    return TRUE;
  }
  return FALSE;
}

void logo_draw_cb (MyLogo *self, cairo_t *cr) {
  guint32 w, h;
  cairo_surface_t *surf = cairo_surface_reference (
      g_object_get_data (self, "surf"));
  if (surf == NULL) return;
  w = cairo_image_surface_get_width (surf);
  h = cairo_image_surface_get_height (surf);
  gtk_window_resize (gtk_widget_get_toplevel (self), w, h);
  cairo_set_source_surface (cr, surf, 0, 0);
  cairo_paint (cr);
  cairo_surface_destroy (surf);
}

void close_window_cb (GtkWidget *widget, MyLogo *logo) {
  OutWindow *post;
  cairo_surface_t *surf = g_object_get_data (logo, "surf");
  gpointer deleted = g_object_get_data (logo, "deleted"); //检测后处理是否被删除，NULL则还没有删除。
  GtkWindow *win = gtk_widget_get_toplevel (logo);
  if (surf != NULL) cairo_surface_destroy (surf);
  if (deleted == NULL) {
    post = g_object_get_data (logo, "post");
    while(g_async_queue_remove(post->com.widget_draw_queue, post->display_widget));
    post->display_widget = NULL;      //通知后处理窗口已经关闭
  }
}

void create_display_widget (OutWindow *post) {
  GtkWindow *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated (win, FALSE);
  MyLogo *logo = my_logo_new (win, TRUE);
  gtk_container_add (win, logo);
  post->display_widget = logo;
  g_signal_connect(logo, "logo_draw", logo_draw_cb, NULL);
  GtkButton *button = gtk_button_new_from_icon_name (
      "window-close-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_relief (button, GTK_RELIEF_NONE);
  my_logo_pack (logo, button, 0, 0, 32, 32, FALSE);
  g_signal_connect_swapped(button, "clicked", gtk_widget_destroy, win);
  g_signal_connect(win, "destroy", close_window_cb, post->display_widget);
  g_object_set_data (post->display_widget, "post", post);
  gtk_widget_show_all (win);
}

gboolean out_windows (OutWindow *post, cairo_surface_t **s, gpointer *out) {
  cairo_surface_t *surf;
  if (*s == NULL) return FALSE;
  guint32 w, h;
  w = cairo_image_surface_get_width (*s);
  h = cairo_image_surface_get_height (*s);
  if (post->display_widget == NULL) return FALSE;
  surf = g_object_get_data (post->display_widget, "surf");
  g_object_set_data (post->display_widget, "surf",
		     cairo_surface_reference (*s));
  if (post->com.widget_draw_queue != NULL){
    while(g_async_queue_remove(post->com.widget_draw_queue, post->display_widget));
    g_async_queue_push (post->com.widget_draw_queue, post->display_widget);
  }
  //gtk_widget_queue_draw(post->display_widget);
  if (surf != NULL) cairo_surface_destroy (surf);
  return TRUE;
}

gboolean out_file (OutFile *file, cairo_surface_t **s, gpointer *out) {
  gchar *str;
  GString *note;
  gsize len;
  guint8 *data = *out;
  if (data == NULL) return FALSE;
  if (file->out == NULL) return FALSE;
  GOutputStream *o = file->out;
  PostTransferData *t = file->com.transferdata;
  if (file->c_source) {
    note = g_string_new ("");
    g_string_append_printf (
	note,
	"\n//index:%ld\tid:%d\tname:%s\twidth:%d\theight:%d\tsize:%d\tframerate:%d/%d\n",
	file->index, file->com.area_id, file->com.name, t->w, t->h,
	t->output_size, t->framerate_n, t->framerate_d);
    if (file->head_output) {
      g_string_append_printf (note,
			      "\t0x%02x,0x%02x,0x%02x,0x%02x,\t//id=0x%08x\n",
			      file->com.area_id & 0xff,
			      (file->com.area_id >> 8) & 0xff,
			      (file->com.area_id >> 16) & 0xff,
			      (file->com.area_id >> 24) & 0xff,
			      file->com.area_id);
      g_string_append_printf (
	  note, "\t0x%02x,0x%02x,0x%02x,0x%02x,\t//width=0x%08x\n", t->w & 0xff,
	  (t->w >> 8) & 0xff, (t->w >> 16) & 0xff, (t->w >> 24) & 0xff, t->w);
      g_string_append_printf (
	  note, "\t0x%02x,0x%02x,0x%02x,0x%02x,\t//height=0x%08x\n",
	  t->h & 0xff, (t->h >> 8) & 0xff, (t->h >> 16) & 0xff,
	  (t->h >> 24) & 0xff, t->h);
      g_string_append_printf (
	  note,
	  "\t0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\t//size=0x%016lx\n",
	  t->output_size & 0xff, (t->output_size >> 8) & 0xff,
	  (t->output_size >> 16) & 0xff, (t->output_size >> 24) & 0xff,
	  (t->output_size >> 32) & 0xff, (t->output_size >> 40) & 0xff,
	  (t->output_size >> 48) & 0xff, (t->output_size >> 56) & 0xff,
	  t->output_size);
      g_string_append_printf (note,
			      "\t0x%02x,0x%02x,\t//framerate=0x%02x/0x%02x",
			      t->framerate_n, t->framerate_d, t->framerate_n,
			      t->framerate_d);
    }
    len = img_data_to_c_array_string (data, t->output_size, 16, note->str,
				      &str);
    g_output_stream_write (o, str, len, NULL, NULL);
    g_string_free (note, TRUE);
    g_free (str);
  }
  else if (file->asm_source) {
    note = g_string_new ("");
    g_string_append_printf (
	note,
	"\n;index:%ld\tid:%d\tname:%s\twidth:%d\theight:%d\tsize:%d\tframerate:%d/%d\n",
	file->index, file->com.area_id, file->com.name, t->w, t->h,
	t->output_size, t->framerate_n, t->framerate_d);
    if (file->head_output) {
      g_string_append_printf (note,
			      "\tDB %02XH,%02XH,%02XH,%02XH\t;id=0x%08x\n",
			      file->com.area_id & 0xff,
			      (file->com.area_id >> 8) & 0xff,
			      (file->com.area_id >> 16) & 0xff,
			      (file->com.area_id >> 24) & 0xff,
			      file->com.area_id);
      g_string_append_printf (note,
			      "\tDB %02XH,%02XH,%02XH,%02XH\t;width=0x%08x\n",
			      t->w & 0xff, (t->w >> 8) & 0xff,
			      (t->w >> 16) & 0xff, (t->w >> 24) & 0xff, t->w);
      g_string_append_printf (note,
			      "\tDB %02XH,%02XH,%02XH,%02XH\t;height=0x%08x\n",
			      t->h & 0xff, (t->h >> 8) & 0xff,
			      (t->h >> 16) & 0xff, (t->h >> 24) & 0xff, t->h);
      g_string_append_printf (
	  note,
	  "\tDB %02XH,%02XH,%02XH,%02XH,%02XH,%02XH,%02XH,%02XH\t;size=0x%016lx\n",
	  t->output_size & 0xff, (t->output_size >> 8) & 0xff,
	  (t->output_size >> 16) & 0xff, (t->output_size >> 24) & 0xff,
	  (t->output_size >> 32) & 0xff, (t->output_size >> 40) & 0xff,
	  (t->output_size >> 48) & 0xff, (t->output_size >> 56) & 0xff,
	  t->output_size);
      g_string_append_printf (note,
			      "\tDB %02XH,%02XH\t;framerate=0x%02x/0x%02x",
			      t->framerate_n, t->framerate_d, t->framerate_n,
			      t->framerate_d);
    }
    len = img_data_to_asm_db_string (data, t->output_size, 16, note->str, &str);
    g_output_stream_write (o, str, len, NULL, NULL);
    g_string_free (note, TRUE);
    g_free (str);
  }
  else {
    if (file->head_output) {
      g_output_stream_write (o, &file->com.area_id, sizeof(guint32), NULL,
      NULL);
      g_output_stream_write (o, &t->w, sizeof(guint32), NULL, NULL);
      g_output_stream_write (o, &t->h, sizeof(guint32), NULL, NULL);
      g_output_stream_write (o, &t->output_size, sizeof(guint64), NULL,
      NULL);
      g_output_stream_write (o, &t->framerate_n, sizeof(guint8),
      NULL,
			     NULL);
      g_output_stream_write (o, &t->framerate_d, sizeof(guint8),
      NULL,
			     NULL);
    }
    g_output_stream_write (o, data, t->output_size, NULL, NULL);
  }
  file->index++;
  return TRUE;
}

gboolean out_img_file (OutImgFile *img, cairo_surface_t **s, gpointer *out) {
  cairo_surface_t *surf = *s;
  if (surf == NULL) return FALSE;
  gchar *num = g_strdup_printf ("%d", img->index);
  gchar *id = g_strdup_printf ("%d", img->com.area_id);
  gchar *name = g_strdup_printf ("%s%s%s", img->directory, G_DIR_SEPARATOR_S,
				 img->name_fmt);
  string_replace (&name, "%d", num);
  string_replace (&name, "%a", id);
  img->index++;
  cairo_surface_write_to_png (surf, name);
  g_free (name);
  g_free (id);
  g_free (num);
  return TRUE;
}
