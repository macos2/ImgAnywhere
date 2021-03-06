/*
 * Img2Format.c
 *
 *  Created on: 2021年6月21日
 *      Author: tom
 */

#include "ImgFormat.h"

uint64_t img_bit_map_format (uint8_t *in, uint8_t **out, uint32_t w, uint32_t h,uint8_t step) {
  uint32_t j;
  uint64_t res=0;
  if(step==0)step=1;
  uint32_t ow=(w+step*8-1)/(step*8)*step;	//output width size in byte, align step byte.
  uint32_t iw=(w+31)/32*4;		//input image width size in byte,align 4 byte. Cairo A1 Format.
  uint8_t *o=malloc(ow*h);
  *out=o;
  uint8_t *i=in;
  for(j=0;j<h;j++){
      memcpy(o,i,ow);
      res+=ow;
      //pointer move to next row;
      o+=ow;
      i+=iw;
  }
  return res;
}

uint64_t img_rgb_format (uint32_t *rgb_in, uint8_t **out, uint32_t w, uint32_t h,
			RGBFormat format) {
  uint8_t *s=rgb_in,t;
  uint8_t *d=NULL;
  int32_t i=w*h,j;
  size_t size;
  switch (format) {
    case RGB_FORMAT_444:
    	size=(((i+1))/2)*3;
      d=malloc(size);
      *out=d;
      for(j=0;j<i;j+=2){//2 pixel each loop
	d[0]=s[2]*16/256;//R0
	d[0]=d[0]<<4;
	d[0]|=s[1]*16/256;//G0
	d[1]=s[0]*16/256;//B0
	d[1]=d[1]<<4;
	d[1]|=s[6]*16/256;//R1
	d[2]=s[5]*16/256;//G1
	d[2]=d[2]<<4;
	d[2]|=s[4]*16/256;//B1
	d+=3;
	s+=8;
      }
      break;
    case RGB_FORMAT_565:
    	size=i*2;
      d=malloc(size);
      *out=d;
      for(j=0;j<i;j++){//1 pixel each loop
	d[0]=s[2]*32/256;//R
	d[0]=d[0]<<3;
	t=s[1]*32/256;
	d[0]|=t>>3;//G
	d[1]=t<<5;//G
	d[1]|=s[0]*32/256;//B
	s+=4;
	d+=2;
      }
      break;
    case RGB_FORMAT_666:
    	size=i*3;
      d=malloc(size);
      *out=d;
      for(j=0;j<i;j++){//1 pixel each loop
	d[0]=(s[2]*64/256)<<2;//R
	d[1]=(s[1]*64/256)<<2;//G
	d[2]=(s[0]*64/256)<<2;//B
	d+=3;
	s+=4;
      }
      break;
    case RGB_FORMAT_888:
    	size=i*3;
      d=malloc(size);
      *out=d;
      for(j=0;j<i;j++){//1 pixel each loop
	d[0]=s[2];//R
	d[1]=s[1];//G
	d[2]=s[0];//B
	d+=3;
	s+=4;
      }
      break;
    case RGB_FORMAT_ARGB32:
    default:
    	size=i*4;
    	d=malloc(size);
    	*out=d;
    	memcpy(d,s,i*4);
    	d+=i*4;
    	break;
  }
  return size;
}

uint64_t img_data_to_string(uint8_t *data,char *note,uint64_t length,uint32_t col,char *row_start,char *fmt,char *seperate,char **res){
  uint64_t i;
  uint64_t l=0;
  char *fmt_s;
  char *temp=NULL,*temp2;
  asprintf(&fmt_s,"%s%s","%s",fmt);
  l=asprintf(&temp,"%s",note);
  for(i=0;i<length;i++){
    if(i%col==0){
      l=asprintf(&temp2,"%s\n",temp);
      free(temp);
      temp=temp2;
      if(row_start!=NULL){
          l=asprintf(&temp2,"%s%s",temp,row_start);
          free(temp);
          temp=temp2;
      }
    }
    l=asprintf(&temp2,fmt_s,temp,data[i]);
    free(temp);
    temp=temp2;
    if((i+1)%col!=0){
        l=asprintf(&temp2,"%s%s",temp,seperate);
        free(temp);
        temp=temp2;
    }
  }
  *res=temp;
  free(fmt_s);
  return l;
}

uint64_t img_data_to_c_array_string(uint8_t *data,uint64_t length,uint32_t col,char *note,char **res){
  uint64_t l=0;
  char *n=malloc(strlen(note)+3);
  if(note==NULL){
    sprintf(n,"");
  }else{
    sprintf(n,"%s",note);
  }
  l= img_data_to_string(data, n, length, col, "\t", "0x%02X,", "\0", res);
  free(n);
  return l;
}

uint64_t img_data_to_asm_db_string(uint8_t *data,uint64_t length,uint32_t col,char *note,char **res){
  uint64_t l=0;
  char *n=malloc(strlen(note)+3);
  if(note==NULL){
    sprintf(n,"");
  }else{
    sprintf(n,"%s",note);
  }
  l= img_data_to_string(data, n, length, col, "\tDB ", "%02XH", ",", res);
  free(n);
  return l;
}
