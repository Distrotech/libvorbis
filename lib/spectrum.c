/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: spectrum envelope and residue code/decode
 last mod: $Id: spectrum.c,v 1.10 2000/01/05 03:11:03 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <math.h>
#include "vorbis/codec.h"

#include "os.h"
#include "bitwise.h"
#include "spectrum.h"

/* this code is still seriously abbreviated.  I'm filling in pieces as
   we go... --Monty 19991004 */

/* unlike other LPC-based coders, we never apply the filter but only
   inspect the frequency response, thus we don't need to guard against
   instability.  However, two coefficients quantising to the same
   value will cause the response to explode.  */

int _vs_spectrum_encode(vorbis_block *vb,double amp,double *lsp){
  /* no real coding yet.  Just write out full sized words for now
     because people need bitstreams to work with */

  int scale=vb->W;
  int m=vb->vd->vi->floororder[scale];
  int n=vb->pcmend*64;
  int last=0;
  double dlast=0.;
  double min=M_PI/n/2.;
  
  int bits=rint(log(n)/log(2));
  int i;

#ifdef TRAIN
  if(amp>0){
    FILE *out;
    if(vb->W)
      out=fopen("lspcoeff-long.vqd","a");
    else
      out=fopen("lspcoeff-short.vqd","a");

    for(i=0;i<m;i++)
      fprintf(out,"%lf ",lsp[i]);
    fprintf(out,"\n");
    fclose(out);

    if(vb->W)
      out=fopen("lspamp-long.vqd","a");
    else
      out=fopen("lspamp-short.vqd","a");
    fprintf(out,"%lf\n",amp);
    fclose(out);
  }
#endif
 
  _oggpack_write(&vb->opb,amp*32768,18);
  
  for(i=0;i<m;i++){
    int val=rint(lsp[i]/M_PI*n-last);
    _oggpack_write(&vb->opb,val,bits);

    lsp[i]=(last+=val)*M_PI/n;

    /* Underpowered but sufficient for now. In the real spec (coming
       soon), a distance of zero can't happen. */
    if(lsp[i]<dlast+min)lsp[i]=dlast+min;
    dlast=lsp[i];
  }
  return(0);
}

int _vs_spectrum_decode(vorbis_block *vb,double *amp,double *lsp){
  int scale=vb->W;
  int m=vb->vd->vi->floororder[scale];
  int n=vb->pcmend*64;
  int last=0;
  double dlast=0.;
  int bits=rint(log(n)/log(2));
  int i;
  double min=M_PI/n/2.;

  *amp=_oggpack_read(&vb->opb,18)/32768.;

  for(i=0;i<m;i++){
    int val=_oggpack_read(&vb->opb,bits);
    lsp[i]=(last+=val)*M_PI/n;

    /* Underpowered but sufficient */
    if(lsp[i]<dlast+min)lsp[i]=dlast+min;
    dlast=lsp[i];
  }
  return(0);
}

void _vs_residue_train(vorbis_block *vb,double *data,double *curve,int n){
  int i;
  FILE *out;
  if(vb->W)
    out=fopen("residue-long.vqd","a");
  else
    out=fopen("residue-short.vqd","a");
      
  for(i=0;i<n;i++){
    double val=0;
    if(curve[i]!=0.)val=data[i]/curve[i];
    fprintf(out,"%lf ",val);
  }
  fprintf(out,"\n");
  fclose(out);
} 

void _vs_residue_quantize(double *data,double *curve,
				 vorbis_info *vi,int n){

  /* The following is temporary, hardwired bullshit */
  int i;

  for(i=0;i<n;i++){
    int val=0;

    if(curve[i]!=0.)val=rint(data[i]/curve[i]);
    if(val>31)val=31;
    if(val<-31)val=-31;


    /*if(val==0){
      if(data[i]<0){
	val=-1;
      }else{
	val=1;
      }
      }*/
    
    data[i]=val;

  }
}

int _vs_residue_encode(vorbis_block *vb,double *data){
  /* no real coding yet.  Just write out full sized words for now
     because people need bitstreams to work with */

  int              n=vb->pcmend/2;
  int i;

  for(i=0;i<n;i++){
    _oggpack_write(&vb->opb,(int)(data[i]+31),6);
  }

  return(0);
}

int _vs_residue_decode(vorbis_block *vb,double *data){
  /* no real coding yet.  Just write out full sized words for now
     because people need bitstreams to work with */

  int              n=vb->pcmend/2;
  int i;

  for(i=0;i<n;i++){
    data[i]=_oggpack_read(&vb->opb,6)-31;
    /*if(data[i]>=0)data[i]+=1;*/
  }
  return(0);
}

