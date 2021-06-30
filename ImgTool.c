/*
 * ImgTool.c
 *
 *  Created on: 2021年6月18日
 *      Author: tom
 */

#include "ImgTool.h"


void img_argb_to_gray(uint8_t *argb_img, uint8_t *gray_img,
		uint32_t w,uint32_t h, MeanOpt mean) {
	uint32_t i;
	uint32_t argb_len=w*h;
	uint8_t *s = argb_img;
	uint8_t *d = gray_img;
	unsigned int temp;
	double x;
	for (i = 0; i < argb_len; i ++) {
		switch (mean) {
		case MEAN_NUM:
			temp=(s[0]+s[1]+s[2])/3;
			*d=temp;
			break;
		case MEAN_GEO:
		default:
			x=s[0]*s[0]+s[1]*s[1]+s[2]*s[2];
			temp=ceil(sqrt(x)*255/442);
			*d=temp;
			break;
		}
		d++;
		s+=4;
	}
}

/*
 * A=AR*R+AG*G+AB*B+AA*A+AC
 * R=RR*R+RG*G+RB*B+RA*A+RC
 * G=GR*R+GG*G+GB*B+GA*A+GC
 * B=BR*R+BG*G+BB*B+BA*A+BC
 */
void img_argb_remap(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,RemapWeight *weight){
	uint8_t *s=in,*r,*g,*b,*a;
	uint8_t *d=out;
	uint32_t i=w*h;
	uint16_t temp;
while(i){
	a=&s[3];
	r=&s[2];
	g=&s[1];
	b=&s[0];
	temp=weight->AA**a+weight->AB**b+weight->AG**g+weight->AR**r+weight->AC;
	d[3]=temp>0xff?0xff:temp;
	temp=weight->RA**a+weight->RB**b+weight->RG**g+weight->RR**r+weight->RC;
	d[2]=temp>0xff?0xff:temp;
	temp=weight->GA**a+weight->GB**b+weight->GG**g+weight->GR**r+weight->GC;
	d[1]=temp>0xff?0xff:temp;
	temp=weight->BA**a+weight->BB**b+weight->BG**g+weight->BR**r+weight->BC;
	d[0]=temp>0xff?0xff:temp;
	s+=4;
	d+=4;
	i--;
}
}

void img_gray_to_bit(uint8_t *gray_img, uint32_t *bit_img,
		uint32_t w,uint32_t h, uint8_t threshold) {
	uint32_t i,j=0;
	uint8_t k;
	uint32_t *d=bit_img;
	uint8_t *s=gray_img;
	uint32_t _w=ceil(w/32.);//width length should alien 32bit
	for(i=0;i<_w*h;i++){
		*d=0;
		for(k=0;k<32;k++){
			*d=*d>>1;
			if(*s>threshold)*d|=0x80000000;
			s++;
			j++;
			if(j>=w){
				j=0;
				break;
			}
		}
		d++;
	}
}


void img_argb_color_inv(uint8_t *argb,uint32_t w,uint32_t h){
  uint32_t i=w*h,j;
  for(j=0;j<i;j++){
    argb[0]=~argb[0];
    argb[1]=~argb[1];
    argb[2]=~argb[2];
    argb+=4;//next pixel ,each pixel 4 byte.
  }
}

void img_byte_inv(uint8_t *img,uint32_t w,uint32_t h,uint8_t pixel_size){
  uint32_t i=w*h*pixel_size,j;
  for(j=0;j<i;j++){
    img[j]=~img[j];
  }
}

void img_rank(uint8_t *in, uint8_t *out,
		uint32_t w_byte,uint32_t h, uint8_t rank){
	uint8_t *s=in;
	uint8_t *d=out;
	uint32_t i=w_byte*h;
	uint8_t u=256/rank;
	while(i){
		*d=*s-*s%u;
		d++;
		s++;
		i--;
	}
}

void img_error_diffusion(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,uint8_t pixel_size,uint8_t rank,DiffRatio *ratio){
	uint8_t e;
	uint32_t i,j;
	uint32_t _w=w*pixel_size;
	if(rank<=0)rank=1;
	uint8_t u=255/rank;
	uint16_t div=0,temp;
	div=ratio->bm+ratio->r+ratio->rb;
	for(i=0;i<h;i++){
		for(j=0;j<_w;j++){
			e=(in[i*_w+j]+out[i*_w+j])%u;
			temp=in[i*_w+j]+out[i*_w+j]-e;
			out[i*_w+j]=temp>255?255:temp;
			if(j<(_w-pixel_size)){
				out[i*_w+j+pixel_size]+=e*ratio->r/div;//误差的扩散至右像素。
				if(i<(h-1))
					out[(i+1)*_w+j+pixel_size]+=e*ratio->rb/div;//误差的扩散至右下像素。
				}
			if(i<(h-1))out[(i+1)*_w+j]+=e*ratio->bm/div;//误差的扩散至下边像素。
			}
		}
	}

void img_edge_detect(uint8_t *in,uint8_t *out,uint32_t w,uint32_t h,uint8_t pixel_size){
	uint32_t i,j;
	uint32_t _w=w*pixel_size;
	for(i=0;i<h;i++){
		for(j=0;j<_w;j++){
			if(j<(_w-pixel_size))out[i*_w+j]+=abs(in[i*_w+j+pixel_size]-in[i*_w+j]);
			if(i<(h-1))out[i*_w+j]+=abs(in[(i+1)*_w+j]-in[i*_w+j]);
		}
	}
}
