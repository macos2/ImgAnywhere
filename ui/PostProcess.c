/*
 * PostProcess.c
 *
 *  Created on: 2021年7月29日
 *      Author: tom
 */


#include "PostProcess.h"

PostProcessFunc get_post_process_func(PostCommon *post){
	switch(post->post_type){
	case POST_BITMAP:
	case POST_DIFFUSE:
	case POST_GRAY:
	case POST_RGB_FMT:
	case POST_ARGB_REMAP:
	case POST_TRANSPARENT:
	case OUT_FILE:
	case OUT_IMG_FILE:
	case OUT_WINDOWS:
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

gboolean post_process(PostCommon *post,gpointer data,gpointer *out){
	gboolean ret=FALSE;
	PostProcessFunc func=get_post_process_func(post);
	ret=func(post,data,out);
	return ret;
}

gboolean post_gray(PostGray *gray,gpointer data,gpointer *out){
	img_argb_to_gray(data,data, gray->com.w, gray->com.h, gray->mean);
	*out=NULL;
	return TRUE;
}

gboolean post_rgb_fmt(PostRGBFmt *rgb_fmt,gpointer data,gpointer *out){
	img_rgb_format(data, out, rgb_fmt->com.w,rgb_fmt->com.h,rgb_fmt->fmt);
	return TRUE;
}

gboolean post_argb_remap(PostARGBRemap *argbremap,gpointer data,gpointer *out){
	img_argb_remap(data, data, argbremap->com.w, argbremap->com.h, &argbremap->remap_weight);
	*out=NULL;
	return TRUE;
}

gboolean post_diffuse(PostDiffuse *diffuse,gpointer data,gpointer *out){
	img_error_diffusion(data, data, diffuse->com.w, diffuse->com.h, diffuse->pixal_size, diffuse->rank, diffuse->radio);
	return TRUE;
}

gboolean post_transparent(PostTransparent *transparent,gpointer data,gpointer *out){
	gdouble d=0;
	guint32 size, i;
	guint8 *p=data;
	size=transparent->com.w*transparent->com.h;
	for(i=0;i<size;i++){
		d=sqrt(pow(p[0]-transparent->b,2.)+pow(p[1]-transparent->g,2.)+pow(p[2]-transparent->r,2.)+pow(p[3]-transparent->a,2.));
		if(d<=transparent->color_distance){
			d=d/transparent->color_distance;
			p[0]*=d;
			p[1]*=d;
			p[2]*=d;
			p[3]=0xff*d;
		}
		p+=4;
	}
	*out=NULL;
	return TRUE;
}

gboolean post_bitmap(PostBitmap *bitmap,gpointer data,gpointer *out){
	guint8 *gray=g_malloc(bitmap->com.w*bitmap->com.h+1);
	img_argb_to_gray(data, gray, bitmap->com.w, bitmap->com.h, MEAN_NUM);
	img_gray_to_bit(gray_img, bit_img, w, h, threshold)
	return TRUE;
}
gboolean out_windows(OutWindow *window,gpointer data,gpointer *out){
	gboolean ret=FALSE;
	return ret;
}
gboolean out_file(OutFile *file,gpointer data,gpointer *out){
	gboolean ret=FALSE;
	return ret;
}
gboolean out_img_file(OutImgFile *img,gpointer data,gpointer *out){
	gboolean ret=FALSE;
	return ret;
}
