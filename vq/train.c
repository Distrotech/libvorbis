/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: utility main for training codebooks
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Dec 15 1999

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "vqgen.h"
#include "vqext.h"

static char *linebuffer=NULL;
static int  lbufsize=0;
static char *rline(FILE *in,FILE *out,int pass){
  long sofar=0;
  if(feof(in))return NULL;

  while(1){
    int gotline=0;

    while(!gotline){
      if(sofar>=lbufsize){
	if(!lbufsize){	
	  lbufsize=1024;
	  linebuffer=malloc(lbufsize);
	}else{
	  lbufsize*=2;
	  linebuffer=realloc(linebuffer,lbufsize);
	}
      }
      {
	long c=fgetc(in);
	switch(c){
	case '\n':
	case EOF:
	  gotline=1;
	  break;
	default:
	  linebuffer[sofar++]=c;
	  linebuffer[sofar]='\0';
	  break;
	}
      }
    }
    
    if(linebuffer[0]=='#'){
      if(pass)fprintf(out,"%s",linebuffer);
    }else{
      return(linebuffer);
    }
  }
}

/* command line:
   trainvq [vq=file | [entries=n] [dim=n] [quant=n]] in=file,firstcol 
           [in=file,firstcol]
*/

int exiting=0;
void setexit(int dummy){
  fprintf(stderr,"\nexiting... please wait to finish this iteration\n");
  exiting=1;
}

int main(int argc,char *argv[]){
  vqgen v;
  quant_return q;

  int entries=-1,dim=-1,quant=-1;
  FILE *out=NULL;
  char *line;
  long i,j,k;

  double desired=.05;
  int iter=1000;

  int init=0;
  while(*argv){

    /* continue training an existing book */
    if(!strncmp(*argv,"vq=",3)){
      FILE *in=NULL;
      char filename[80],*ptr;
      if(sscanf(*argv,"vq=%70s",filename)!=1){
	fprintf(stderr,"Syntax error in argument '%s'\n",*argv);
	exit(1);
      }

      in=fopen(filename,"r");
      ptr=strrchr(filename,'-');
      if(ptr){
	int num;
	ptr++;
	num=atoi(ptr);
	sprintf(ptr,"%d.vqi",num+1);
      }else
	strcat(filename,"-0.vqi");
      
      out=fopen(filename,"w");
      if(out==NULL){
	fprintf(stderr,"Unable to open %s for writing\n",filename);
	exit(1);
      }

      if(in){
	/* we wish to suck in a preexisting book and continue to train it */
	double a;

	line=rline(in,out,1);
	if(strlen(line)>0)line[strlen(line)-1]='\0';
	if(strcmp(line,vqext_booktype)){
	  fprintf(stderr,"wrong book type; %s!=%s\n",line,vqext_booktype);
	  exit(1);
	} 
	    
	line=rline(in,out,1);
	if(sscanf(line,"%d %d",&entries,&dim)!=2){
	  fprintf(stderr,"Syntax error reading book file\n");
	  exit(1);
	}
	

	vqgen_init(&v,dim,entries,vqext_metric);
	init=1;

	/* quant setup */
	line=rline(in,out,1);
	if(sscanf(line,"%lf %lf %d %d",&q.minval,&q.delt,
		  &q.addtoquant,&quant)!=4){
	  fprintf(stderr,"Syntax error reading book file\n");
	  exit(1);
	}

	/* entries */
	i=0;
	for(j=0;j<entries;j++){
	  for(k=0;k<dim;k++){
	    line=rline(in,out,0);
	    sscanf(line,"%lf",&a);
	    v.entrylist[i++]=a;
	  }
	}
	
	/* dequantize */
	vqext_unquantize(&v,&q);

	/* bias, points */
	i=0;
	for(j=0;j<entries;j++){
	  line=rline(in,out,0);
	  sscanf(line,"%lf",&a);
	  v.bias[i++]=a;
	}

	{
	  double b[80];
	  i=0;
	  v.entries=0; /* hack to avoid reseeding */
	  while(1){
	    for(k=0;k<dim && k<80;k++){
	      line=rline(in,out,0);
	      sscanf(line,"%lf",b+k);
	    }
	    if(feof(in))break;
	    vqgen_addpoint(&v,b);
	  }
	  v.entries=entries;
	}

	fclose(in);
      }
    }

    /* set parameters if we're not loading a pre book */
    if(!strncmp(*argv,"quant=",6)){
      sscanf(*argv,"quant=%d",&quant);
    }
    if(!strncmp(*argv,"entries=",8)){
      sscanf(*argv,"entries=%d",&entries);
    }
    if(!strncmp(*argv,"dim=",4)){
      sscanf(*argv,"dim=%d",&dim);
    }
    if(!strncmp(*argv,"desired=",8)){
      sscanf(*argv,"desired=%lf",&desired);
    }
    if(!strncmp(*argv,"iter=",5)){
      sscanf(*argv,"iter=%d",&iter);
    }

    if(!strncmp(*argv,"in=",3)){
      int start;
      char file[80];
      FILE *in;
      int cols=-1;

      if(sscanf(*argv,"in=%79[^,],%d",file,&start)!=2)goto syner;
      if(!out){
	fprintf(stderr,"vq= must preceed in= arguments\n");
	exit(1);
      }
      if(!init){
	if(dim==-1 || entries==-1 || quant==-1){
	  fprintf(stderr,"Must specify dimensionality,entries,quant before"
		  " first input file\n");
	  exit(1);
	}
	vqgen_init(&v,dim,entries,vqext_metric);
	init=1;
      }

      in=fopen(file,"r");
      if(in==NULL){
	fprintf(stderr,"Could not open input file %s\n",file);
	exit(1);
      }
      fprintf(out,"# training file entry: %s\n",file);

      while((line=rline(in,out,1))){
	if(cols==-1){
	  char *temp=line;
	  while(*temp==' ')temp++;
	  for(cols=0;*temp;cols++){
	    while(*temp>32)temp++;
	    while(*temp==' ')temp++;
	  }
	}
	{
	  int i;
	  double *b=alloca(cols*sizeof(double));
	  if(start+dim>cols){
	    fprintf(stderr,"ran out of columns reading %s\n",file);
	    exit(1);
	  }
	  while(*line==' ')line++;
	  for(i=0;i<cols;i++){
	    b[i]=atof(line);
	    while(*line>32)line++;
	    while(*line==' ')line++;
	  }
	  vqext_adjdata(b,start,dim);
	  vqgen_addpoint(&v,b+start);
	}
      }
      fclose(in);
    }
    argv++;
  }

  /* train the book */
  signal(SIGTERM,setexit);
  signal(SIGINT,setexit);

  for(i=0;i<iter && !exiting;i++){
    double result;
    if(i!=0)vqext_unquantize(&v,&q);
    result=vqgen_iterate(&v);
    q=vqext_quantize(&v,quant);
    if(result<desired)break;
  }

  /* save the book */

  fprintf(out,"# OggVorbis VQ codebook trainer, intermediate file\n");
  fprintf(out,"%s\n",vqext_booktype);
  fprintf(out,"%d %d\n",entries,dim);
  fprintf(out,"%g %g %d %d\n",q.minval,q.delt,q.addtoquant,quant);

  i=0;
  for(j=0;j<entries;j++)
    for(k=0;k<dim;k++)
      fprintf(out,"%f\n",v.entrylist[i++]);
  
  fprintf(out,"# biases---\n");
  i=0;
  for(j=0;j<entries;j++)
    fprintf(out,"%f\n",v.bias[i++]);

  fprintf(out,"# points---\n");
  i=0;
  for(j=0;j<v.points;j++)
    for(k=0;k<dim && k<80;k++)
      fprintf(out,"%f\n",v.pointlist[i++]);

  fclose(out);
  exit(0);

  syner:
    fprintf(stderr,"Syntax error in argument '%s'\n",*argv);
    exit(1);
}
