/*
 * main.c
 *
 *  Created on: 2021年6月10日
 *      Author: tom
 */
#include <gtk/gtk.h>
#include "MyVideoArea.h"
#include "ImgTool.h"
gdouble angle;

gboolean rotate_func(MyVideoArea *area){
	angle+=1.;
	if(angle>360.)angle=0.;
//	VideoBoxArea *box=my_video_area_get_area(area,"test");
//	box->rotateAngle=angle/180.*G_PI;
//	box=my_video_area_get_area(area,"test2");
//	box->rotateAngle=angle/180.*G_PI;
//	my_video_area_rotate_area_by_name(area,"test",FIX_LEFT_TOP,-5.);
//	my_video_area_rotate_area_by_name(area,"test2",FIX_RIGHT_TOP,-5.);
//	my_video_area_rotate_area_by_name(area,"test3",FIX_RIGHT_BOTTOM,-5.);
	my_video_area_move_area_by_name(area,"test",1.,0.);
	gtk_widget_queue_draw(area);
	return G_SOURCE_CONTINUE;
}

int main (int argc,char *argv[]){
	gtk_init(&argc,&argv);
	GtkWidget *win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	MyVideoArea *area=my_video_area_new();
	angle=0.;
	my_video_area_add_area(area,"test",300.,100.,30.,30.);
	my_video_area_add_area(area,"test2",200.,100.,30.,30.);
	//my_video_area_add_area(area,"test3",1200.,200.,400.,400.);
	my_video_area_add_area(area,"test3",500.,800.,400.,400.);
	my_video_area_add_area(area,"test4",300.,100.,30.,30.);
	my_video_area_add_area(area,"test5",200.,100.,30.,30.);
	my_video_area_add_area(area,"test6",100,100.,100.,100.);
	//my_video_area_rotate_area_by_name(area,"test3",FIX_CENTRE,180.);
//	my_video_area_add_area(area,"test4",100,100.,100.,10.,20.);
//	my_video_area_add_area(area,"test5",100,100.,100.,10.,30.);
//	my_video_area_add_area(area,"test6",100,100.,100.,10.,0.);
//	my_video_area_add_area(area,"test7",50,50.,100.,100.,0.);
	//g_timeout_add(100,rotate_func,area);
	GdkPixbuf *buf=gdk_pixbuf_new_from_file("/home/tom/c76c9b015a35563e6e29673f46847753.jpg",NULL);
	//GdkPixbuf *buf=gdk_pixbuf_new_from_file("/home/tom/b26d0c84ed83d360f967199618a17aea.jpg",NULL);

	//GdkPixbuf *buf=gdk_pixbuf_new_from_file("/home/tom/无标题.png",NULL);
	my_video_area_set_pixbuf(area,buf);
	my_video_area_set_scale(area,1.);
	g_object_unref(buf);
	gtk_container_add(win,area);
	g_signal_connect(win,"delete-event",gtk_main_quit,NULL);
	gtk_widget_show_all(win);

	cairo_surface_t *s=my_video_area_get_area_content_by_name(area,"test3");
	//img_rank(cairo_image_surface_get_data(s),cairo_image_surface_get_data(s),400*4,400,2);
	cairo_surface_write_to_png(s,"result.png");
	cairo_surface_t *color_diff=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,400,400);
	cairo_surface_t *color_channel=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,400,400);
	img_error_diffusion(cairo_image_surface_get_data(s),cairo_image_surface_get_data(color_diff),400,400,4,1,&diff_332);
	cairo_surface_write_to_png(color_diff,"color_diff.png");
	argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&a_channel);
	cairo_surface_write_to_png(color_channel,"alpha.png");
	argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&r_channel);
	cairo_surface_write_to_png(color_channel,"red.png");
	argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&g_channel);
	cairo_surface_write_to_png(color_channel,"green.png");
	argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&b_channel);
	cairo_surface_write_to_png(color_channel,"blue.png");
	//img_edge_detect(cairo_image_surface_get_data(s),cairo_image_surface_get_data(color_diff),400,400,4);

	cairo_surface_t *gray=cairo_image_surface_create(CAIRO_FORMAT_A8,400,400);
	argb_to_gray(cairo_image_surface_get_data(s),cairo_image_surface_get_data(gray),400,400,MEAN_NUM);
	cairo_surface_write_to_png(gray,"gray.png");

	cairo_surface_t *gray_diff=cairo_image_surface_create(CAIRO_FORMAT_A8,400,400);
	//img_error_diffusion(cairo_image_surface_get_data(gray),cairo_image_surface_get_data(gray_diff),400,400,1,1,&diff_332);
	img_edge_detect(cairo_image_surface_get_data(gray),cairo_image_surface_get_data(gray_diff),400,400,1);
	cairo_surface_write_to_png(gray_diff,"gray_diff.png");

	cairo_surface_t *bit=cairo_image_surface_create(CAIRO_FORMAT_A1,400,400);
	gray_to_bit(cairo_image_surface_get_data(gray_diff),cairo_image_surface_get_data(bit),400,400,100);
	cairo_surface_write_to_png(bit,"bit.png");

	gtk_main();
	return 0;
}
