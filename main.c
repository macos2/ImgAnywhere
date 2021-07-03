/*
 * main.c
 *
 *  Created on: 2021年6月10日
 *      Author: tom
 */
#include <gtk/gtk.h>

#include "ImgFormat.h"
#include "SurfFormat.h"
#include "ImgTool.h"
#include "ui/MyMain.h"
#include "ui/MyVideoArea.h"
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


void imgtool_test(cairo_surface_t *s){
	cairo_surface_t *color_diff=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,400,400);
	cairo_surface_t *color_channel=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,400,400);
	img_error_diffusion(cairo_image_surface_get_data(s),cairo_image_surface_get_data(color_diff),400,400,4,1,&diff_332);
	cairo_surface_write_to_png(color_diff,"color_diff.png");
	img_argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&a_channel);
	cairo_surface_write_to_png(color_channel,"alpha.png");
	img_argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&r_channel);
	cairo_surface_write_to_png(color_channel,"red.png");
	img_argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&g_channel);
	cairo_surface_write_to_png(color_channel,"green.png");
	img_argb_remap(cairo_image_surface_get_data(color_diff),cairo_image_surface_get_data(color_channel),400,400,&b_channel);
	cairo_surface_write_to_png(color_channel,"blue.png");
	//img_edge_detect(cairo_image_surface_get_data(s),cairo_image_surface_get_data(color_diff),400,400,4);

	cairo_surface_t *gray=cairo_image_surface_create(CAIRO_FORMAT_A8,400,400);
	img_argb_to_gray(cairo_image_surface_get_data(s),cairo_image_surface_get_data(gray),400,400,MEAN_NUM);
	cairo_surface_write_to_png(gray,"gray.png");

	cairo_surface_t *gray_diff=cairo_image_surface_create(CAIRO_FORMAT_A8,400,400);
	//img_error_diffusion(cairo_image_surface_get_data(gray),cairo_image_surface_get_data(gray_diff),400,400,1,1,&diff_332);
	img_edge_detect(cairo_image_surface_get_data(gray),cairo_image_surface_get_data(gray_diff),400,400,1);
	cairo_surface_write_to_png(gray_diff,"gray_diff.png");

	cairo_surface_t *bit=cairo_image_surface_create(CAIRO_FORMAT_A1,400,400);
	img_gray_to_bit(cairo_image_surface_get_data(gray_diff),cairo_image_surface_get_data(bit),400,400,100);
	cairo_surface_write_to_png(bit,"bit.png");
}

void img2bin_test(cairo_surface_t *s){
  uint8_t *p=NULL;
  uint32_t w,h;
  uint64_t l;
  w=cairo_image_surface_get_width(s);
  h=cairo_image_surface_get_height(s);
  l=img_rgb_format(cairo_image_surface_get_data(s), &p, w, h, RGB_FORMAT_444);
  g_file_set_contents("rgb444.bin", p, l, NULL);
  g_free(p);
  l=img_rgb_format(cairo_image_surface_get_data(s), &p, w, h, RGB_FORMAT_565);
  g_file_set_contents("rgb565.bin", p, l, NULL);
  g_free(p);
  l=img_rgb_format(cairo_image_surface_get_data(s), &p, w, h, RGB_FORMAT_666);
  g_file_set_contents("rgb666.bin", p, l, NULL);
  g_free(p);
  l=img_rgb_format(cairo_image_surface_get_data(s), &p, w, h, RGB_FORMAT_888);
  g_file_set_contents("rgb888.bin", p, l, NULL);
  g_free(p);
}

void b2bin_test(){
  uint32_t i,j;
  uint64_t l;
  cairo_surface_t *surf=cairo_image_surface_create(CAIRO_FORMAT_A8, 128,64);
  cairo_t *cr=cairo_create(surf);
  cairo_arc(cr, 32., 32., 10., 0,G_PI);
  cairo_set_source_rgb(cr, 1., 1.,1.);
  cairo_set_line_width(cr, 2.);
  cairo_stroke(cr);
  cairo_surface_flush(surf);
  cairo_destroy(cr);
  cairo_surface_write_to_png(surf, "A8.png");
  cairo_surface_t *argb=surf_a8_to_rgba_surf(surf,  0xFF000000,0xFFFFFFFF);
  cairo_surface_write_to_png(argb, "argb.png");

  g_file_set_contents("A1.bin", cairo_image_surface_get_data(surf),64*cairo_image_surface_get_stride(surf), NULL);
  guint8 *p;
//  l=img_bit_map_format(cairo_image_surface_get_data(surf), &p, 70,64, 1);
//  g_file_set_contents("bit8.bin", p,l , NULL);
//  g_free(p);
//  l=img_bit_map_format(cairo_image_surface_get_data(surf), &p, 70,64, 2);
//  g_file_set_contents("bit16.bin", p,l , NULL);
//  g_free(p);
//  l=img_bit_map_format(cairo_image_surface_get_data(surf), &p, 70,64, 4);
//  g_file_set_contents("bit32.bin", p,l , NULL);
//  g_free(p);

//  l=surf_a1_transform_by_scan(surf,&p, SCAN_DIR_LEFT_TO_RIGHT, SCAN_DIR_TOP_TO_BOTTOM, 0, BIT_DIR_VERTICAL);
//  g_file_set_contents("scan.bin", p,l , NULL);
//  g_free(p);

  l=img_data_to_c_array_string(cairo_image_surface_get_data(surf), 64*cairo_image_surface_get_stride(surf), 16, "nothing",&p);
  g_print("C:\r\n%s\r\n",p);
  g_free(p);

  l=img_data_to_asm_db_string(cairo_image_surface_get_data(surf), 64*cairo_image_surface_get_stride(surf), 16, "nothing",&p);
  g_print("ASM:\r\n%s\r\n",p);
  g_free(p);
}

void image_convert(const char *path){
  guint64 l;
  guint8 *p;
  gchar *str;
  GdkPixbuf *pix=gdk_pixbuf_new_from_file(path, NULL);
  cairo_surface_t *argb=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 128, 64);
  cairo_surface_t *gray=cairo_image_surface_create(CAIRO_FORMAT_A8, 128, 64);
  cairo_surface_t *bit=cairo_image_surface_create(CAIRO_FORMAT_A1, 128, 64);
  cairo_t *cr=cairo_create(argb);
  gdouble r=64./(gdk_pixbuf_get_height(pix)*1.);
  cairo_translate(cr, (128.-gdk_pixbuf_get_width(pix)*r)/2., 0);
  cairo_scale(cr, r, r);
  gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);
  cairo_surface_write_to_png(argb, "dog-argb.png");
  img_argb_to_gray(cairo_image_surface_get_data(argb), cairo_image_surface_get_data(gray), 128, 64, MEAN_NUM);
  cairo_surface_mark_dirty (gray);
  cairo_surface_write_to_png(gray, "dog-gray.png");
  img_gray_to_bit(cairo_image_surface_get_data(gray), cairo_image_surface_get_data(bit),128, 64, 128);
  cairo_surface_write_to_png(bit, "dog-bit.png");
  l=surf_a1_transform_by_scan(bit, &p, SCAN_DIR_LEFT_TO_RIGHT, SCAN_DIR_TOP_TO_BOTTOM, Byte_LSB_FIRST, BIT_DIR_VERTICAL);
  img_data_to_c_array_string(p, l, 16, "nothing",&str);
  g_print(str);

}


int main (int argc,char *argv[]){
	gtk_init(&argc,&argv);
	/*
	GtkWidget *win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	MyVideoArea *area=my_video_area_new();
	angle=0.;
	my_video_area_add_area(area,"test","lock",300.,100.,30.,30.);
	my_video_area_add_area(area,"test2","unlock",200.,100.,30.,30.);
	//my_video_area_add_area(area,"test3",1200.,200.,400.,400.);
	my_video_area_add_area(area,"test3",NULL,500.,800.,400.,400.);
	my_video_area_add_area(area,"test4",NULL,300.,100.,30.,30.);
	my_video_area_add_area(area,"test5",NULL,200.,100.,30.,30.);
	my_video_area_add_area(area,"test6",NULL,100,100.,100.,100.);
	//my_video_area_rotate_area_by_name(area,"test3",FIX_CENTRE,180.);
//	my_video_area_add_area(area,"test4",100,100.,100.,10.,20.);
//	my_video_area_add_area(area,"test5",100,100.,100.,10.,30.);
//	my_video_area_add_area(area,"test6",100,100.,100.,10.,0.);
//	my_video_area_add_area(area,"test7",50,50.,100.,100.,0.);
	//g_timeout_add(100,rotate_func,area);
	GdkPixbuf *buf=gdk_pixbuf_new_from_file("dog.jpg",NULL);

	//buf=gdk_pixbuf_new_from_data(data, colorspace, has_alpha, bits_per_sample, width, height, rowstride, destroy_fn, destroy_fn_data);



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
//	imgtool_test(s);
//	img2bin_test(s);
	 b2bin_test();
	 //image_convert("dog.jpg");
	 */
	 MyMain *w=g_object_new(MY_TYPE_MAIN,NULL);
	 gtk_widget_show_all(w);
	gtk_main();
	return 0;
}
