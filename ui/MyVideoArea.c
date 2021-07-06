/*
 * MyVideoArea.c
 *
 *  Created on: 2021年6月10日
 *      Author: tom
 */

#include "MyVideoArea.h"

#include "math.h"

typedef struct {
  GList *area_list;
  guint autoindex;
  GdkPixbuf *pixbuf;
  gdouble mouseX, mouseY, pressX, pressY;
  gboolean pressed;
  VideoBoxArea *pre_sel_area, *sel_area;
  FixPoint pre_point, sel_point;
  guint8 select_index;
  gdouble scale;
  cairo_matrix_t org_matrix;
} MyVideoAreaPrivate;

G_DEFINE_TYPE_WITH_CODE(MyVideoArea, my_video_area, GTK_TYPE_DRAWING_AREA,
			G_ADD_PRIVATE(MyVideoArea));

void fix_point_rotate (gdouble x, gdouble y, gdouble angle, cairo_matrix_t *mat) {
  cairo_matrix_translate (mat, x, y);
  cairo_matrix_rotate (mat, angle);
  cairo_matrix_translate (mat, -1. * x, -1. * y);
}

gdouble point_distance (gdouble x, gdouble y, gdouble x0, gdouble y0) {
  gdouble d = (x - x0) * (x - x0) + (y - y0) * (y - y0);
  return sqrt (d);
}

/* overridable methods */
void to_area_coodinate (cairo_t *cr, VideoBoxArea *area, gdouble *mouseX,
			gdouble *mouseY,MyVideoArea *self) {
  cairo_matrix_t a,b, res;
  GtkAllocation alloc;
  MyVideoAreaPrivate *priv=my_video_area_get_instance_private(self);
  //matrix multiply
  cairo_translate (cr, area->offsetX*priv->scale, area->offsetY*priv->scale);
  cairo_get_matrix (cr, &a);
  cairo_matrix_multiply (&res, &area->obj_mat, &a);
  cairo_matrix_translate (&res, area->w*priv->scale / (-2.), area->h*priv->scale / (-2.));
  cairo_set_matrix (cr, &res);
  //transform the mouse x,y offset.
  if (mouseX == NULL || mouseY == NULL) return;
  cairo_matrix_init_translate(&a,  area->offsetX*priv->scale, area->offsetY*priv->scale);
  cairo_matrix_multiply(&b, &priv->org_matrix, &a);
  cairo_matrix_multiply (&res, &area->obj_mat, &b);
  cairo_matrix_translate (&res, area->w*priv->scale / (-2.), area->h*priv->scale / (-2.));
  cairo_matrix_invert (&res);
  cairo_matrix_transform_point (&res, mouseX, mouseY);
}

void draw_area (cairo_t *cr, VideoBoxArea *area,gdouble scale,gdouble r, gdouble g,
		gdouble b, gdouble a) {
  cairo_rectangle (cr, 0, 0, area->w*scale, area->h*scale);
  cairo_set_source_rgba (cr, 1., 1, 1, 0.8);
  cairo_set_line_width (cr, 3.);
  cairo_stroke_preserve (cr);
  cairo_set_source_rgba (cr, r, g, b, a);
  cairo_set_line_width (cr, 1.);
  cairo_stroke (cr);
  //draw triangle mark
  cairo_move_to (cr, area->w*scale, area->h*scale - 5.);
  cairo_line_to (cr, area->w*scale - 5., area->h*scale);
  cairo_line_to (cr, area->w*scale, area->h*scale);
  cairo_close_path (cr);
  cairo_set_source_rgba (cr, 1., 1, 1, 0.8);
  cairo_set_line_width (cr, 3.);
  cairo_stroke_preserve (cr);
  cairo_set_source_rgba (cr, r, g, b, a);
  cairo_fill (cr);
}

gboolean my_video_area_draw_area (cairo_t *cr, MyVideoArea *self) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  guint8 i = 0;
  cairo_text_extents_t ex;
  gdouble mouseX, mouseY;
  GList *l=priv->area_list;
  VideoBoxArea *area;
  gchar *text;
  priv->pre_sel_area = NULL;

  //translate to the pixbuf org
  cairo_matrix_t res,tmp;
  cairo_get_matrix(cr, &tmp);
  cairo_matrix_multiply(&res, &tmp, &priv->org_matrix);
  cairo_set_matrix(cr, &res);
  while (l != NULL) {
    cairo_save (cr);
    area = l->data;
    mouseX = priv->mouseX, mouseY = priv->mouseY;
    to_area_coodinate (cr, area, &mouseX, &mouseY,self);

    //draw the all the area label
    if (area->describe != NULL) {
      text = g_strdup_printf ("%s-%s", area->label, area->describe);
    }
    else {
      text = g_strdup_printf ("%s", area->label);
    }
    cairo_set_font_size (cr, 12.);
    cairo_text_extents (cr, text, &ex);
    cairo_rectangle (cr, 0., (ex.height + 10.) * -1., ex.width + 10.,
		     ex.height + 10.);
    cairo_set_source_rgba (cr, 0.5, 0., 0.8, 0.5);
    cairo_fill (cr);
    cairo_set_source_rgba (cr, 1., 1., 1., 1.);
    cairo_save (cr);
    cairo_translate (cr, 5., -5.);
    cairo_show_text (cr, text);
    cairo_fill (cr);
    cairo_restore (cr);
    g_free (text);
    //mouse in area detect,skip the normal draw process if the mouse in it or the selected area
    cairo_rectangle (cr, -3., -3., area->w*priv->scale + 6., area->h*priv->scale + 6.);
    if (cairo_in_fill (cr, mouseX, mouseY)) {
      cairo_new_path (cr);
      if (priv->select_index == 0) {
	if (priv->pre_sel_area != NULL) {
	  cairo_restore (cr);
	  cairo_save (cr);
	  to_area_coodinate (cr, priv->pre_sel_area, NULL, NULL,self);
	  if (priv->sel_area != priv->pre_sel_area)
	    draw_area (cr, priv->pre_sel_area,priv->scale ,0., 0., 1., 0.6);
	}
	priv->pre_sel_area = area;
      }
      else {
	if (i == priv->select_index) {
	  priv->pre_sel_area = area;
	}
	else {
	  cairo_new_path (cr);
	  if (priv->sel_area != area) draw_area (cr, area, priv->scale,0., 0., 1., 0.6);
	}
	i++;
      }
    }
    else {
      cairo_new_path (cr);
      if (priv->sel_area != area) draw_area (cr, area,  priv->scale,0., 0., 1., 0.6);
    }
    cairo_restore (cr);
    l = l->next;
  }

  //draw the area which the mouse in.
  if (priv->pre_sel_area != NULL) {
    GdkPixbuf *buf = NULL;
    cairo_save (cr);
    area = priv->pre_sel_area;
    mouseX = priv->mouseX, mouseY = priv->mouseY;
    to_area_coodinate (cr, priv->pre_sel_area, &mouseX, &mouseY,self);
    cairo_set_source_rgba (cr, 0.2, 0.2, 1., 1.);
    if (point_distance (0, 0, mouseX, mouseY) < 4.) {
      priv->pre_point = FIX_LEFT_TOP;
      buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				      "view-refresh-symbolic", 16,
				      GTK_ICON_LOOKUP_FORCE_SYMBOLIC, NULL);
      gdk_cairo_set_source_pixbuf (cr, buf, -8, -8);
      cairo_paint (cr);
      g_object_unref (buf);
    }
    else if (point_distance (0, area->h*priv->scale, mouseX, mouseY) < 4.) {
      priv->pre_point = FIX_LEFT_BOTTOM;
      buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				      "view-refresh-symbolic", 16,
				      GTK_ICON_LOOKUP_FORCE_SYMBOLIC, NULL);
      gdk_cairo_set_source_pixbuf (cr, buf, -8, area->h*priv->scale - 8);
      cairo_paint (cr);
      g_object_unref (buf);
    }
    else if (point_distance (area->w*priv->scale, 0, mouseX, mouseY) < 4.) {
      priv->pre_point = FIX_RIGHT_TOP;
      buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				      "view-refresh-symbolic", 16,
				      GTK_ICON_LOOKUP_FORCE_SYMBOLIC, NULL);
      gdk_cairo_set_source_pixbuf (cr, buf, area->w*priv->scale - 8, -8);
      cairo_paint (cr);
      g_object_unref (buf);
    }
    else if (point_distance (area->w*priv->scale, area->h*priv->scale, mouseX, mouseY) < 4.) {
      priv->pre_point = FIX_RIGHT_BOTTOM;
      buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				      "view-fullscreen-symbolic", 16,
				      GTK_ICON_LOOKUP_FORCE_SYMBOLIC, NULL);
      gdk_cairo_set_source_pixbuf (cr, buf, area->w*priv->scale - 8, area->h*priv->scale - 8);
      cairo_paint (cr);
      g_object_unref (buf);
    }
    else if (point_distance (area->w*priv->scale / 2., area->h*priv->scale / 2., mouseX, mouseY) < 4.) {
      priv->pre_point = FIX_CENTRE;
      buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				      "view-refresh-symbolic", 16,
				      GTK_ICON_LOOKUP_FORCE_SYMBOLIC, NULL);
      gdk_cairo_set_source_pixbuf (cr, buf, area->w*priv->scale / 2. - 8, area->h*priv->scale / 2. - 8);
      cairo_paint (cr);
      g_object_unref (buf);
    }
    else {
      priv->pre_point = FIX_NONE;
    }
    cairo_restore (cr);
    cairo_save (cr);
    to_area_coodinate (cr, priv->pre_sel_area, NULL, NULL,self);
    if (priv->sel_area != priv->pre_sel_area) {
      draw_area (cr, area, priv->scale, 0., 1., 0., 0.6);
    }
    cairo_restore (cr);
  }
  //draw the selected area.
  cairo_save (cr);
  if (priv->sel_area != NULL) {
    mouseX = priv->mouseX, mouseY = priv->mouseY;
    to_area_coodinate (cr, priv->sel_area, &mouseX, &mouseY,self);
    draw_area (cr, priv->sel_area, priv->scale, 1., 0., 0., 0.6);
    cairo_move_to (cr, priv->sel_area->w*priv->scale / 2. - 4., priv->sel_area->h*priv->scale / 2.);
    cairo_line_to (cr, priv->sel_area->w*priv->scale / 2. + 4., priv->sel_area->h*priv->scale / 2.);
    cairo_move_to (cr, priv->sel_area->w*priv->scale / 2., priv->sel_area->h*priv->scale / 2. - 4.);
    cairo_line_to (cr, priv->sel_area->w*priv->scale / 2., priv->sel_area->h*priv->scale / 2. + 4.);
    //cairo_set_line_width(cr,3.);
    cairo_stroke (cr);
  }
  cairo_restore (cr);

  return FALSE;
}

gboolean my_video_area_draw (MyVideoArea *self, cairo_t *cr) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  VideoBoxArea *area;
  priv->pre_sel_area = NULL;
  GtkAllocation alloc;
  gint w=0, h=0;
  gtk_widget_get_allocation (self, &alloc);
  cairo_save (cr);
  //draw background
  if (priv->pixbuf != NULL) {
    w = gdk_pixbuf_get_width (priv->pixbuf) * priv->scale;
    h = gdk_pixbuf_get_height (priv->pixbuf) * priv->scale;
    cairo_translate (cr, (alloc.width - w) / 2., (alloc.height - h) / 2.);
    cairo_scale (cr, priv->scale, priv->scale);
    gdk_cairo_set_source_pixbuf (cr, priv->pixbuf, 0., 0.);
    if( priv->scale>1.)cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
    cairo_paint (cr);
  }
  cairo_restore (cr);

  cairo_matrix_init_translate(&priv->org_matrix, (alloc.width - w) / 2., (alloc.height - h) / 2.);
  cairo_save (cr);
  my_video_area_draw_area (cr, self);
  cairo_restore (cr);
  return TRUE;
}

void my_video_area_set_property (MyVideoArea *self, guint property_id,
				 const GValue *value, GParamSpec *pspec) {

}
void my_video_area_get_property (MyVideoArea *self, guint property_id,
				 GValue *value, GParamSpec *pspec) {

}
void my_video_area_dispose (MyVideoArea *self) {

}

void my_video_area_finalize (MyVideoArea *self) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  GList *l = priv->area_list;
  VideoBoxArea *area = NULL;
  while (l != NULL) {
    area=l->data;
    g_free(area->describe);
    g_free(area->label);
    g_free(area);
    l = l->next;
  }
  g_list_free (priv->area_list);
  gdk_pixbuf_unref (priv->pixbuf);
}

/*signal*/
gboolean my_video_area_pointer_motion (MyVideoArea *self, GdkEventMotion *event,
				       gpointer user_data) {
  cairo_matrix_t mat, res;
  gdouble x, y;
  gdouble x0, y0;
  gdouble w, h;
  gdouble orig_x, orig_y;
  gdouble angle, angle0;
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  if (priv->pressed && priv->sel_area != NULL) {
    if (priv->sel_point == FIX_NONE) {
      //move area
      my_video_area_move_area (self, priv->sel_area, (event->x - priv->pressX)/priv->scale,
			       (event->y - priv->pressY)/priv->scale);
    }
    else if (priv->sel_point == FIX_RIGHT_BOTTOM) {
      //resize area
      x = event->x;
      y = event->y;
      x0 = priv->pressX;
      y0 = priv->pressY;
      cairo_matrix_init_translate (&mat, priv->sel_area->offsetX,
				   priv->sel_area->offsetY);
      cairo_matrix_multiply (&res, &priv->sel_area->obj_mat, &mat);
      cairo_matrix_translate (&res, priv->sel_area->w / (-2.),
			      priv->sel_area->h / (-2.));
      cairo_matrix_transform_point (&res, &x, &y);
      cairo_matrix_transform_point (&res, &x0, &y0);
      w = (x - x0)/priv->scale * 2.;
      h = (y - y0)/priv->scale * 2.;
      g_signal_emit_by_name (self, "area_resize", priv->sel_area, &w, &h, NULL);
      priv->sel_area->w += w;
      priv->sel_area->h += h;
      if (priv->sel_area->w < 8.) priv->sel_area->w = 8.;
      if (priv->sel_area->h < 8.) priv->sel_area->h = 8.;
    }
    else {
      //rotate area
      x = event->x;
      y = event->y;
      x0 = priv->pressX;
      y0 = priv->pressY;
      cairo_matrix_init_translate (&mat, priv->sel_area->offsetX,
				   priv->sel_area->offsetY);
      cairo_matrix_multiply (&res, &priv->sel_area->obj_mat, &mat);
      cairo_matrix_translate (&res, priv->sel_area->w / (-2.),
			      priv->sel_area->h / (-2.));
      cairo_matrix_transform_point (&res, &x, &y);
      cairo_matrix_transform_point (&res, &x0, &y0);
      orig_x = priv->sel_area->w;
      orig_y = priv->sel_area->h;
      switch (priv->sel_point) {
	case FIX_LEFT_TOP:
	  orig_x *= -0.5;
	  orig_y *= -0.5;
	  break;
	case FIX_LEFT_BOTTOM:
	  orig_x *= -0.5;
	  orig_y *= 0.5;
	  break;
	case FIX_RIGHT_TOP:
	  orig_x *= 0.5;
	  orig_y *= -0.5;
	  break;
	case FIX_RIGHT_BOTTOM: //it will not execute
	  orig_x *= 0.5;
	  orig_y *= 0.5;
	  break;
	default:
	  orig_x = 0;
	  orig_y = 0;
      }
      x -= orig_x;
      y -= orig_y;
      x0 -= orig_x;
      y0 -= orig_y;
      angle = atan (y / x);
      angle0 = atan (y0 / x0);
      //g_printf("angel=%lf\r\n",(angle0-angle)/G_PI*180.);
      angle = (angle0 - angle) > 0 ? -2.5 : 2.5;
      my_video_area_rotate_area (self, priv->sel_area, priv->sel_point, angle);
    }
    priv->pressX = event->x;
    priv->pressY = event->y;
  }
  else {
    priv->mouseX = event->x;
    priv->mouseY = event->y;
  }
  gtk_widget_queue_draw (self);
  return TRUE;
}

gboolean my_video_area_pointer_press (MyVideoArea *self, GdkEventButton *event,
				      gpointer user_data) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  priv->pressed = TRUE;
  priv->pressX = event->x;
  priv->pressY = event->y;
  if(event->button==3)
	  priv->select_index++;
  else
	  priv->select_index = 0;
  if (priv->pre_sel_area != NULL) {
    if (priv->pre_sel_area != priv->sel_area) {
      g_signal_emit_by_name (self, "area_unselect", priv->sel_area);
      g_signal_emit_by_name (self, "area_select", priv->pre_sel_area);
    }
    priv->sel_area = priv->pre_sel_area;
    priv->sel_point = priv->pre_point;
  }

  return FALSE;
}

gboolean my_video_area_pointer_release (MyVideoArea *self,
					GdkEventButton *event,
					gpointer user_data) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  priv->pressed = FALSE;
  priv->pressX = -1.;
  priv->pressY = -1.;
  return FALSE;
}

gboolean my_video_area_scroll (MyVideoArea *self, GdkEventScroll *event,
			       gpointer user_data) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  switch (event->direction) {
    case GDK_SCROLL_UP:
      priv->select_index++;
      break;
    case GDK_SCROLL_DOWN:
    default:
      if (priv->select_index > 0) priv->select_index--;
      break;
  }
  gtk_widget_queue_draw (self);
  return FALSE;
}

static void my_video_area_class_init (MyVideoAreaClass *klass) {
  GObjectClass *obj_class = klass;
  GtkWidgetClass *widget_class = klass;
  obj_class->set_property = my_video_area_set_property;
  obj_class->get_property = my_video_area_get_property;
  obj_class->dispose = my_video_area_dispose;
  obj_class->finalize = my_video_area_finalize;
  widget_class->draw = my_video_area_draw;
  g_signal_new ("area_select", MY_TYPE_VIDEO_AREA, G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET(MyVideoAreaClass, area_select), NULL, NULL,
		NULL, G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
  g_signal_new ("area_unselect", MY_TYPE_VIDEO_AREA, G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET(MyVideoAreaClass, area_unselect), NULL, NULL,
		NULL, G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
  g_signal_new ("area_rotate", MY_TYPE_VIDEO_AREA, G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET(MyVideoAreaClass, area_rotate), NULL, NULL,
		NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER, NULL);
  g_signal_new ("area_move", MY_TYPE_VIDEO_AREA, G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET(MyVideoAreaClass, area_move), NULL, NULL, NULL,
		G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
		NULL);
  g_signal_new ("area_resize", MY_TYPE_VIDEO_AREA, G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET(MyVideoAreaClass, area_resize), NULL, NULL,
		NULL, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER,
		G_TYPE_POINTER, NULL);
}

static void my_video_area_init (MyVideoArea *self) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  priv->area_list = NULL;
  priv->autoindex = 0;
  priv->pixbuf = NULL;
  priv->sel_area = NULL;
  priv->pre_sel_area = NULL;
  priv->pressed = FALSE;
  priv->scale = 1.;
  gtk_widget_add_events (
      self,
      GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
	  | GDK_SCROLL_MASK);
  g_signal_connect(self, "motion-notify-event", my_video_area_pointer_motion,
		   NULL);
  g_signal_connect(self, "button-press-event", my_video_area_pointer_press,
		   NULL);
  g_signal_connect(self, "button-release-event", my_video_area_pointer_release,
		   NULL);
  g_signal_connect(self, "scroll-event", my_video_area_scroll, NULL);
}

MyVideoArea* my_video_area_new () {
  MyVideoArea *area = g_object_new (MY_TYPE_VIDEO_AREA, NULL);
  return area;
}

VideoBoxArea * my_video_area_add_area (MyVideoArea *self, gchar *label, gchar *desribe,
			     gfloat x, gfloat y, gfloat w, gfloat h) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  VideoBoxArea *area = g_malloc (sizeof(VideoBoxArea));
  gchar *temp;
  area->h = h;
  area->offsetX = x;
  area->offsetY = y;
  area->w = w;
  area->describe = g_strdup (desribe);
  cairo_matrix_init_rotate (&area->obj_mat, 0.);
  if (label == NULL) {
    temp = g_strdup_printf ("%d", priv->autoindex);
    area->label=temp;
    priv->autoindex++;
  }
  else {
    area->label = g_strdup (label);
  }
  priv->area_list=g_list_append(priv->area_list, area);
  return area;
}

void my_video_area_remove_area (MyVideoArea *self, gchar *label) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  VideoBoxArea *area=NULL;
  GList *l=priv->area_list;
  while(l!=NULL){
    area=l->data;
    if(g_strcmp0(label, area->label)==0){
      priv->area_list=g_list_remove(priv->area_list, area);
      g_free (area->describe);
      g_free (area->label);
      g_free (area);
      break;
    }
    l=l->next;
  }
  gtk_widget_queue_draw (self);
}

void my_video_area_rename_area (MyVideoArea *self, gchar *old_label,
				gchar *new_label) {
  if(old_label==NULL||new_label==NULL)return;
  VideoBoxArea *area;
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  GList *l=priv->area_list;
  while(l!=NULL){
    area=l->data;
    if(g_strcmp0(old_label, area->label)==0){
      g_free (area->label);
      area->label = g_strdup (new_label);
      break;
    }
    l=l->next;
  }
  gtk_widget_queue_draw (self);
}

VideoBoxArea* my_video_area_get_area (MyVideoArea *self, gchar *label) {
  if(label==NULL)return NULL;
  VideoBoxArea *area = NULL;
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  GList *l=priv->area_list;
  while(l!=NULL){
    area=l->data;
    if(g_strcmp0(label, area->label)==0){
      break;
    }
    l=l->next;
  }
  return area;
}

void my_video_area_move_area (MyVideoArea *self, VideoBoxArea *area, gdouble x,
			      gdouble y) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  g_signal_emit_by_name (self, "area_move", area, &x, &y, NULL);
  area->offsetX += x;
  area->offsetY += y;
}

void my_video_area_move_area_by_name (MyVideoArea *self, gchar *label, gdouble x,
				      gdouble y) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  if (label == NULL) return;
  VideoBoxArea *area = my_video_area_get_area (self, label);
  my_video_area_move_area (self, area, x, y);
  gtk_widget_queue_draw (self);
}

void my_video_area_rotate_area (MyVideoArea *self, VideoBoxArea *area,
				FixPoint point, gdouble angle_degree) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  g_signal_emit_by_name (self, "area_rotate", area, &angle_degree, NULL);
  gdouble angle = (angle_degree / 180.) * G_PI;
  switch (point) {
    case FIX_LEFT_TOP:
      fix_point_rotate (area->w*priv->scale * -0.5, area->h*priv->scale * -0.5, angle, &area->obj_mat);
      break;
    case FIX_LEFT_BOTTOM:
      fix_point_rotate (area->w*priv->scale * -0.5, area->h*priv->scale * 0.5, angle, &area->obj_mat);
      break;
    case FIX_RIGHT_TOP:
      fix_point_rotate (area->w*priv->scale * 0.5, area->h*priv->scale * -0.5, angle, &area->obj_mat);
      break;
    case FIX_RIGHT_BOTTOM:
      fix_point_rotate (area->w*priv->scale * 0.5, area->h*priv->scale * 0.5, angle, &area->obj_mat);
      break;
    default: //FIX_CENTRE
      fix_point_rotate (0., 0., angle, &area->obj_mat);
      break;
  }
}

void my_video_area_rotate_area_by_name (MyVideoArea *self, gchar *label,
					FixPoint point, gfloat angle_degree) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  if (label == NULL) return;
  VideoBoxArea *area = my_video_area_get_area (self, label);
  my_video_area_rotate_area (self, area, point, angle_degree);
  gtk_widget_queue_draw (self);
}

void my_video_area_set_pixbuf (MyVideoArea *self, GdkPixbuf *pixbuf) {
  if (pixbuf == NULL) return;
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  if (priv->pixbuf != NULL) g_object_unref (priv->pixbuf);
  priv->pixbuf = g_object_ref(pixbuf);
  gtk_widget_set_size_request (self,
			       priv->scale * gdk_pixbuf_get_width (pixbuf),
			       priv->scale * gdk_pixbuf_get_height (pixbuf));
}

const GdkPixbuf* my_video_area_get_pixbuf (MyVideoArea *self) {
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  return priv->pixbuf;
}

void my_video_area_set_scale (MyVideoArea *self, gdouble scale) {
  if (scale <= 0) return;
  gint w, h;
  VideoBoxArea *area;
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  GList *l=priv->area_list;
  while(l!=NULL){
	  area=l->data;
	  area->obj_mat.x0*=scale/priv->scale;
	  area->obj_mat.y0*=scale/priv->scale;
	  l=l->next;
  }
  priv->scale = scale;
  if (priv->pixbuf != NULL) {
    w = gdk_pixbuf_get_width (priv->pixbuf);
    h = gdk_pixbuf_get_height (priv->pixbuf);
    gtk_widget_set_size_request (self, w * scale, h * scale);
  }
}

gdouble my_video_area_get_scale(MyVideoArea *self){
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  return priv->scale;
}

cairo_surface_t* my_video_area_get_area_content_by_name (MyVideoArea *self,
							 gchar *label) {
  cairo_matrix_t mat;
  MyVideoAreaPrivate *priv = my_video_area_get_instance_private (self);
  VideoBoxArea *area = my_video_area_get_area (self, label);
  if (area == NULL) return NULL;
  cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						      area->w, area->h);
  cairo_t *cr = cairo_create (surf);
  cairo_save (cr);
  to_area_coodinate (cr, area, NULL, NULL,self);
  cairo_get_matrix (cr, &mat);
  cairo_matrix_invert (&mat);
  cairo_restore (cr);
  cairo_set_matrix (cr, &mat);
  gdk_cairo_set_source_pixbuf (cr, priv->pixbuf, 0., 0.);
  cairo_paint (cr);
  cairo_destroy (cr);
  return surf;
}
