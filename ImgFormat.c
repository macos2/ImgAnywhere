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
  switch (format) {
    case RGB_FORMAT_444:
      d=malloc((((i+1))/2)*3);
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
      d=malloc(i*2);
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
      d=malloc(i*3);
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
    default:
      d=malloc(i*3);
      *out=d;
      for(j=0;j<i;j++){//1 pixel each loop
	d[0]=s[2];//R
	d[1]=s[1];//G
	d[2]=s[0];//B
	d+=3;
	s+=4;
      }
  }

  return d-*out;
}


uint64_t img_data_to_string(uint8_t *data,char *note,uint64_t length,uint32_t col,char *row_start,char *fmt,char *seperate,char **res){
  uint64_t i;
  char *buff;
  uint64_t l=0;
  char tmp_name[]="tmpXXXXXX";
  FILE *f=fdopen(mkstemp(tmp_name),"w+");
  fprintf(f,"%s\r\n",note);
  for(i=0;i<length;i++){
    if(i%col==0){
      l+=fprintf(f,"\r\n");
      if(row_start!=NULL)l+=fprintf(f,"%s",row_start);
    }
    l+=fprintf(f,fmt,data[i]);
    if((i+1)%col!=0)l+=fprintf(f,seperate);
  }
  fflush(f);
  fseek(f,0,SEEK_SET);
  buff=malloc(l+1);
  fread(buff,l,1,f);
  buff[l]=0x00;
  fclose(f);
  unlink(tmp_name);
  *res=buff;
  return l;
}

uint64_t img_data_to_c_array_string(uint8_t *data,uint64_t length,uint32_t col,char *note,char **res){
  uint64_t l=0;
  char *n=malloc(strlen(note)+3);
  if(note==NULL){
    sprintf(n,"");
  }else{
    sprintf(n,"//%s",note);
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
    sprintf(n,";%s",note);
  }
  l= img_data_to_string(data, n, length, col, "DB ", "%02XH", ",", res);
  free(n);
  return l;
}
