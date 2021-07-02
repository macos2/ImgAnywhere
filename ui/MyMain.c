/*
 * MyMain.c
 *
 *  Created on: 2021年7月1日
 *      Author: TOM
 */


#include "MyMain.h"


#define GET_PRIV   MyMainPrivate *priv=my_main_get_instance_private(self)

typedef struct{
  gchar *name;
  VideoBoxArea *area;
  GdkPixbuf *pixbuf;
  GtkListStore *process_list;
}AreaInfo;

typedef struct{
  GtkAdjustment *pos_x,*pos_y,*progress,*rotate,*size_h,*size_w,*volume;
  GtkBox *video_box;
  MyVideoArea *video_area;
  GtkTreeView *modifier_view;
  GtkPaned *paned;
  AreaInfo *current_area;
  GtkToggleButton *size_radio_lock;
}MyMainPrivate;

G_DEFINE_TYPE_WITH_CODE(MyMain,my_main,GTK_TYPE_WINDOW,G_ADD_PRIVATE(MyMain));

enum {
  col_id,
  col_name,
  col_pixbuf,
  col_type,
  col_data,
  num_col,
};

GtkListStore *create_store(){
  GtkListStore *list=gtk_list_store_new(num_col,G_TYPE_UINT,G_TYPE_STRING,GDK_TYPE_PIXBUF,G_TYPE_UINT,G_TYPE_POINTER);
  return list;
}

static void my_main_class_init(MyMainClass *klass){
  size_t s;
  gchar *glade;
  g_file_get_contents("ui/MyMain.glade", &glade, &s, NULL);
  GBytes *b=g_bytes_new(glade, s);
  gtk_widget_class_set_template(klass,b);
  g_bytes_unref(b);
  g_free(glade);
  gtk_widget_class_bind_template_child_private(klass,MyMain,video_box);
  gtk_widget_class_bind_template_child_private(klass,MyMain,pos_x);
  gtk_widget_class_bind_template_child_private(klass,MyMain,pos_y);
  gtk_widget_class_bind_template_child_private(klass,MyMain,progress);
  gtk_widget_class_bind_template_child_private(klass,MyMain,size_w);
  gtk_widget_class_bind_template_child_private(klass,MyMain,size_h);
  gtk_widget_class_bind_template_child_private(klass,MyMain,rotate);
  gtk_widget_class_bind_template_child_private(klass,MyMain,volume);
  gtk_widget_class_bind_template_child_private(klass,MyMain,paned);
  gtk_widget_class_bind_template_child_private(klass,MyMain,modifier_view);
  gtk_widget_class_bind_template_child_private(klass,MyMain,size_radio_lock);
}

static void my_main_init(MyMain *self){
  gint w,h;
  gtk_widget_init_template(self);
  GET_PRIV;
  priv->video_area=my_video_area_new();
  gtk_box_pack_start(priv->video_box, priv->video_area, TRUE, TRUE, 0);
  my_video_area_set_pixbuf(priv->video_area, gdk_pixbuf_new_from_file("dog.jpg", NULL));
  my_video_area_set_scale(priv->video_area, 0.5);
  gtk_widget_get_size_request(priv->video_area,&w,&h);
  gtk_paned_set_position(priv->paned, w);
}
