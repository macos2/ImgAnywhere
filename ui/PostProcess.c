/*
 * PostProcess.c
 *
 *  Created on: 2021年7月29日
 *      Author: tom
 */


#include "PostProcess.h"

PostProcessFunc get_post_process_func(PostCommon *post){
	switch(post->post_type){
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
	case OUT_FILE:
		return out_file;
		break;
	case OUT_IMG_FILE:
		return out_img_file;
		break;
	case OUT_WINDOWS:
		return out_windows;
		break;
	default:break;
	}
	return NULL;
}

void append_post_process(PostCommon *post,PostCommon *append_post){
	post->next_post=append_post;
}

PostCommon *next_post_process(PostCommon *post){
	return post->next_post;
}

gboolean post_process(PostCommon *post,guint8 *data,gpointer *out){
	gboolean ret=FALSE;
	PostProcessFunc func=get_post_process_func(post);
	ret=func(post,data,out);
	return ret;
}

cairo_surface_t *post_preview(PostCommon *post,cairo_surface_t *surf){
	if(surf==NULL)return NULL;
	guint w,h;
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	cairo_surface_t *out=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w,h);
	cairo_t *cr=cairo_create(out);
	cairo_set_source_surface(cr,surf,0,0);
	cairo_paint(cr);
	cairo_surface_flush(out);
	guint8 *data=cairo_image_surface_get_data(out);
	switch (post->post_type){
	case POST_BITMAP:
		PostBitmap *bit=post;
		surf_rgba_to_bw_color(out,MEAN_NUM, bit->thresold);
		break;
	case POST_ARGB_REMAP:
		PostARGBRemap *remap=post;
		img_argb_remap(data, data, w, h, &remap->remap_weight);
		break;
	case POST_DIFFUSE:
		PostDiffuse *diff=post;
		img_error_diffusion(data, data, w, h, 4, diff->rank, &diff->radio);
		break;
	case POST_GRAY:
		PostGray *gray=post;
		img_argb_to_gray(data, data, w, h, gray->mean);
		break;
	case POST_RGB_FMT:
		PostRGBFmt *fmt=post;
		guint8 rank=0;
		switch (fmt->fmt){
		RGB_FORMAT_444:
		rank=16;break;
		RGB_FORMAT_565:
		rank=32;break;
		RGB_FORMAT_666:
		rank=64;break;
		RGB_FORMAT_888:
		default :
			rank=255;
		}
		img_rank(data, data, w*4, h, rank);
		break;
	case POST_TRANSPARENT:

	case POST_BW:
		post_bw(post, out, NULL);
		break;
	case POST_RESIZE:
	default:

	}
	cairo_destroy(cr);
	return out;
}


gboolean post_bw(PostBw *bw,cairo_surface_t *surf,gpointer *out){
	PostCommon *com=&bw->com;
	cairo_surface_flush(surf);
	surf_rgba_to_bw_color(surf, bw->mean,bw->thresold);
	com->out_size=0;
	return TRUE;
}

gboolean post_gray(PostGray *gray,cairo_surface_t *surf,gpointer *out){
	PostCommon *com=&gray->com;
	cairo_surface_flush(surf);
	surf_rgba_to_gray_color(surf, gray->mean);
	com->out_size=0;
	return TRUE;
}

gboolean post_rgb_fmt(PostRGBFmt *rgb_fmt,cairo_surface_t *surf,gpointer *out){
	PostCommon *com=&rgb_fmt->com;
	guint32 w,h,*data=cairo_image_surface_get_data(surf);
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	if(*out!=NULL)g_free(*out);
	com->out_size=img_rgb_format(data, out, w,h,rgb_fmt->fmt);
	com->w=w;
	com->h=h;
	return TRUE;
}

gboolean post_argb_remap(PostARGBRemap *argbremap,cairo_surface_t *surf,gpointer *out){
	PostCommon *com=&argbremap->com;
	guint8 data=cairo_image_surface_get_data(surf);
	guint32 w,h;
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	img_argb_remap(data, data, w, h, &argbremap->remap_weight);
	cairo_surface_mark_dirty(surf);
	com->out_size=0;
	return TRUE;
}

gboolean post_diffuse(PostDiffuse *diffuse,cairo_surface_t *surf,gpointer *out){
	PostCommon *com=&diffuse->com;
	guint8 *data=cairo_image_surface_get_data(surf);
	guint32 w,h;
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	guint8 pixal_size=cairo_image_surface_get_stride(surf)/w;
	img_error_diffusion(data, data, w, h, pixal_size, diffuse->rank, &diffuse->radio);
	cairo_surface_mark_dirty(surf);
	com->out_size=0;
	return TRUE;
}

gboolean post_transparent(PostTransparent *transparent,cairo_surface_t *surf,gpointer *out){
	PostCommon *com=&transparent->com;
	gdouble d=0;
	guint32 size, i;
	guint8 *p=cairo_image_surface_get_data(surf);
	guint32 w,h;
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	size=w*h;
	for(i=0;i<size;i++){
		d=sqrt(pow(p[0]-transparent->b,2.)+pow(p[1]-transparent->g,2.)+pow(p[2]-transparent->r,2.)+pow(p[3]-transparent->a,2.));
		if(d<=transparent->color_distance){
			d=d/transparent->color_distance;
			p[0]=p[0]>transparent->b?p[0]-transparent->b:0;
			p[1]=p[1]>transparent->g?p[1]-transparent->g:0;
			p[2]=p[2]>transparent->r?p[2]-transparent->r:0;
			p[3]=p[3]>transparent->a?p[3]-transparent->a:0;
			p[0]*=d;
			p[1]*=d;
			p[2]*=d;
			p[3]=0xff*d;
		}
		p+=4;
	}
	cairo_surface_mark_dirty(surf);
	com->out_size=0;
	return TRUE;
}

gboolean post_bitmap(PostBitmap *bitmap,cairo_surface_t *surf,gpointer *out){
	PostCommon *com=&bitmap->com;
	cairo_surface_t *argb=NULL,*gray=NULL,*a1=NULL;
	guint32 w,h;
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	switch(cairo_image_surface_get_format(surf)){
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
		argb=cairo_surface_reference(surf);
		gray=cairo_image_surface_create(CAIRO_FORMAT_A8,w,h);
		img_argb_to_gray(cairo_image_surface_get_data(argb), cairo_image_surface_get_data(gray), w, h, MEAN_NUM);
		cairo_surface_mark_dirty(gray);
	case CAIRO_FORMAT_A8:
		if(gray==NULL)gray=cairo_surface_reference(surf);
		a1=cairo_image_surface_create(CAIRO_FORMAT_A1,w,h);
		img_gray_to_bit(cairo_image_surface_get_data(gray), cairo_image_surface_get_data(a1), w, h, bitmap->thresold);
		cairo_surface_mark_dirty(a1);
	case CAIRO_FORMAT_A1:
		if(a1==NULL)a1=cairo_surface_reference(surf);
	default:break;
	}
	if(*out!=NULL)g_free(*out);
	com->out_size=surf_a1_transform_by_scan(a1, out, bitmap->first, bitmap->second, bitmap->order, bitmap->bitdir);
	com->w=w;
	com->h=h;
	cairo_surface_destroy(a1);
	cairo_surface_destroy(gray);
	cairo_surface_destroy(argb);
	return TRUE;
}

gboolean post_resize(PostResize *resize,cairo_surface_t *surf,gpointer *out){
	gdouble wr,hr;
	guint32 w,h;
	if(surf==NULL)return FALSE;
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	wr=resize->resize_w/w;
	hr=resize->resize_h/h;
	if(!resize->expand){
		if(resize->full){
			wr=wr>hr?wr:hr;
			hr=wr;
		}else{
			wr=wr<hr?wr:hr;
			hr=wr;
		}
	}
	cairo_surface_t *osurf=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,resize->resize_w,resize->resize_h);
	cairo_t *cr=cairo_create(osurf);
	cairo_scale(cr,wr,hr);
	cairo_set_source_surface(cr,surf,0,0);
	if(wr>1.)cairo_pattern_set_filter(cairo_get_source(cr),CAIRO_FILTER_NEAREST);
	cairo_paint(cr);
	cairo_destroy(cr);
	if(*out!=NULL)g_free(*out);
	*out=osurf;
	return TRUE;
}

void logo_draw_cb(MyLogo *self,cairo_t *cr){
	cairo_surface_t *surf=g_object_get_data(self,"surf");
	if(surf==NULL)return;
	cairo_set_source_surface(cr,surf,0,0);
	cairo_paint(cr);
}

void close_window_cb(GtkWidget *button,OutWindow *window){
	cairo_surface_t *surf=g_object_get_data(window->display_widget,"surf");
	GtkWindow *win=gtk_widget_get_toplevel(window->display_widget);
	if(surf!=NULL)cairo_surface_destroy(surf);
	window->display_widget=NULL;
	gtk_widget_destroy(win);
}

gboolean out_windows(OutWindow *window,cairo_surface_t *surf,gpointer *out){
	guint32 w,h;
	w=cairo_image_surface_get_width(surf);
	h=cairo_image_surface_get_height(surf);
	if(window->display_widget==NULL){
		GtkWindow *win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_decorated(win,FALSE);
		MyLogo *logo=my_logo_new(win, TRUE);
		gtk_container_add(win,logo);
		window->display_widget=logo;
		g_signal_connect(logo,"logo_draw",logo_draw_cb,NULL);

		GtkButton *button=gtk_button_new_from_icon_name("window-close-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_button_set_relief(button, GTK_RELIEF_NONE);
		my_logo_pack(logo, button, 0, 0, 24, 24, FALSE);
		g_signal_connect(button,"clicked",close_window_cb,window);
		gtk_widget_show_all(win);
		gtk_window_resize(win,w,h);
	}
	if(window->display_widget!=NULL){
		cairo_surface_t *s=g_object_get_data(window->display_widget,"surf");
		if(s!=NULL)cairo_surface_destroy(s);
		g_object_set_data(window->display_widget,"surf",cairo_surface_reference(surf));
		gtk_widget_queue_draw(window->display_widget);
	}
	window->com.out_size=0;
	return TRUE;
}

gboolean out_file(OutFile *file,cairo_surface_t *surf,gpointer *out){
	gchar *str,*note;
	gsize len;
	guint8 *data=*out;
	if(data==NULL)return FALSE;
	if(file->file==NULL){
		if(g_access(file->filename,F_OK|W_OK)==0){
			if(file->over_write)g_unlink(file->filename);
			file->file=g_file_new_for_path(file->filename);
		};
	}
	if(file->file==NULL)return FALSE;
	GOutputStream *o=g_file_append_to(file->file,G_FILE_CREATE_NONE,NULL,NULL);
	if(file->c_source){
		note=g_strdup_printf("//id:\tname:%s\twidth:%d\theight:%d\tsize:%d\n",file->com.id,file->com.name,file->com.w,file->com.h,file->com.out_size);
		len=img_data_to_c_array_string(data, file->com.out_size, 16,note , &str);
		g_output_stream_write(o,str,len,NULL,NULL);
		g_free(note);
		g_free(str);
	}else if(file->asm_source){
		note=g_strdup_printf(";id:\tname:%s\twidth:%d\theight:%d\tsize:%d\n",file->com.id,file->com.name,file->com.w,file->com.h,file->com.out_size);
		len=img_data_to_asm_db_string(data, file->com.out_size, 16,note , &str);
		g_output_stream_write(o,str,len,NULL,NULL);
		g_free(note);
		g_free(str);
	}else{
		g_output_stream_write(o,data,file->com.out_size,NULL,NULL);
	}
	g_output_stream_close(o,NULL,NULL);
	file->com.out_size=0;
	return TRUE;
}
gboolean out_img_file(OutImgFile *img,cairo_surface_t *surf,gpointer *out){
	if(surf==NULL)return FALSE;
	gchar **s;
	gchar **strv=g_strsplit(img->name_fmt, "%d", -1);
	GString *name=g_string_new("");
	s=strv;
	while(*s!=NULL){
		g_string_append(name,*s);
		g_string_append_printf(name,"%d",img->index);
		s++;
	}
	img->index++;
	g_strfreev(strv);
	cairo_surface_write_to_png(surf,name->str);
	g_string_free(name,TRUE);
	img->com.out_size=0;
	return TRUE;
}
