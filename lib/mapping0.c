/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: channel mapping 0 implementation
 last mod: $Id: mapping0.c,v 1.32 2001/06/15 23:59:47 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "codebook.h"
#include "bitbuffer.h"
#include "registry.h"
#include "psy.h"
#include "misc.h"

/* simplistic, wasteful way of doing this (unique lookup for each
   mode/submapping); there should be a central repository for
   identical lookups.  That will require minor work, so I'm putting it
   off as low priority.

   Why a lookup for each backend in a given mode?  Because the
   blocksize is set by the mode, and low backend lookups may require
   parameters from other areas of the mode/mapping */

typedef struct {
  drft_lookup fft_look;
  vorbis_info_mode *mode;
  vorbis_info_mapping0 *map;

  vorbis_look_time **time_look;
  vorbis_look_floor **floor_look;

  vorbis_look_residue **residue_look;
  vorbis_look_psy *psy_look;

  vorbis_func_time **time_func;
  vorbis_func_floor **floor_func;
  vorbis_func_residue **residue_func;

  int ch;
  long lastframe; /* if a different mode is called, we need to 
		     invalidate decay */
} vorbis_look_mapping0;

static vorbis_info_mapping *mapping0_copy_info(vorbis_info_mapping *vm){
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;
  vorbis_info_mapping0 *ret=_ogg_malloc(sizeof(vorbis_info_mapping0));
  memcpy(ret,info,sizeof(vorbis_info_mapping0));
  return(ret);
}

static void mapping0_free_info(vorbis_info_mapping *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_mapping0));
    _ogg_free(i);
  }
}

static void mapping0_free_look(vorbis_look_mapping *look){
  int i;
  vorbis_look_mapping0 *l=(vorbis_look_mapping0 *)look;
  if(l){
    drft_clear(&l->fft_look);

    for(i=0;i<l->map->submaps;i++){
      l->time_func[i]->free_look(l->time_look[i]);
      l->floor_func[i]->free_look(l->floor_look[i]);
      l->residue_func[i]->free_look(l->residue_look[i]);
      if(l->psy_look)_vp_psy_clear(l->psy_look+i);
    }

    _ogg_free(l->time_func);
    _ogg_free(l->floor_func);
    _ogg_free(l->residue_func);
    _ogg_free(l->time_look);
    _ogg_free(l->floor_look);
    _ogg_free(l->residue_look);
    if(l->psy_look)_ogg_free(l->psy_look);
    memset(l,0,sizeof(vorbis_look_mapping0));
    _ogg_free(l);
  }
}

static vorbis_look_mapping *mapping0_look(vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_mapping *m){
  int i;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=vi->codec_setup;
  vorbis_look_mapping0 *look=_ogg_calloc(1,sizeof(vorbis_look_mapping0));
  vorbis_info_mapping0 *info=look->map=(vorbis_info_mapping0 *)m;
  look->mode=vm;
  
  look->time_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_time *));
  look->floor_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_floor *));

  look->residue_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_residue *));
  if(ci->psys)look->psy_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_psy));

  look->time_func=_ogg_calloc(info->submaps,sizeof(vorbis_func_time *));
  look->floor_func=_ogg_calloc(info->submaps,sizeof(vorbis_func_floor *));
  look->residue_func=_ogg_calloc(info->submaps,sizeof(vorbis_func_residue *));
  
  for(i=0;i<info->submaps;i++){
    int timenum=info->timesubmap[i];
    int floornum=info->floorsubmap[i];
    int resnum=info->residuesubmap[i];

    look->time_func[i]=_time_P[ci->time_type[timenum]];
    look->time_look[i]=look->time_func[i]->
      look(vd,vm,ci->time_param[timenum]);
    look->floor_func[i]=_floor_P[ci->floor_type[floornum]];
    look->floor_look[i]=look->floor_func[i]->
      look(vd,vm,ci->floor_param[floornum]);
    look->residue_func[i]=_residue_P[ci->residue_type[resnum]];
    look->residue_look[i]=look->residue_func[i]->
      look(vd,vm,ci->residue_param[resnum]);
    
    if(ci->psys && vd->analysisp){
      int psynum=info->psysubmap[i];
      _vp_psy_init(look->psy_look+i,ci->psy_param[psynum],
		   ci->blocksizes[vm->blockflag]/2,vi->rate);
    }
  }

  look->ch=vi->channels;

  if(vd->analysisp)drft_init(&look->fft_look,ci->blocksizes[vm->blockflag]);
  return(look);
}

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
}

static void mapping0_pack(vorbis_info *vi,vorbis_info_mapping *vm,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;

  /* another 'we meant to do it this way' hack...  up to beta 4, we
     packed 4 binary zeros here to signify one submapping in use.  We
     now redefine that to mean four bitflags that indicate use of
     deeper features; bit0:submappings, bit1:coupling,
     bit2,3:reserved. This is backward compatable with all actual uses
     of the beta code. */

  if(info->submaps>1){
    oggpack_write(opb,1,1);
    oggpack_write(opb,info->submaps-1,4);
  }else
    oggpack_write(opb,0,1);

  if(info->coupling_steps>0){
    oggpack_write(opb,1,1);
    oggpack_write(opb,info->coupling_steps-1,8);
    
    for(i=0;i<info->coupling_steps;i++){
      oggpack_write(opb,info->coupling_mag[i],ilog2(vi->channels));
      oggpack_write(opb,info->coupling_ang[i],ilog2(vi->channels));
    }
  }else
    oggpack_write(opb,0,1);
  
  oggpack_write(opb,0,2); /* 2,3:reserved */

  /* we don't write the channel submappings if we only have one... */
  if(info->submaps>1){
    for(i=0;i<vi->channels;i++)
      oggpack_write(opb,info->chmuxlist[i],4);
  }
  for(i=0;i<info->submaps;i++){
    oggpack_write(opb,info->timesubmap[i],8);
    oggpack_write(opb,info->floorsubmap[i],8);
    oggpack_write(opb,info->residuesubmap[i],8);
  }
}

/* also responsible for range checking */
static vorbis_info_mapping *mapping0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=_ogg_calloc(1,sizeof(vorbis_info_mapping0));
  codec_setup_info     *ci=vi->codec_setup;
  memset(info,0,sizeof(vorbis_info_mapping0));

  if(oggpack_read(opb,1))
    info->submaps=oggpack_read(opb,4)+1;
  else
    info->submaps=1;

  if(oggpack_read(opb,1)){
    info->coupling_steps=oggpack_read(opb,8)+1;

    for(i=0;i<info->coupling_steps;i++){
      int testM=info->coupling_mag[i]=oggpack_read(opb,ilog2(vi->channels));
      int testA=info->coupling_ang[i]=oggpack_read(opb,ilog2(vi->channels));

      if(testM<0 || 
	 testA<0 || 
	 testM==testA || 
	 testM>=vi->channels ||
	 testA>=vi->channels) goto err_out;
    }

  }

  if(oggpack_read(opb,2)>0)goto err_out; /* 2,3:reserved */
    
  if(info->submaps>1){
    for(i=0;i<vi->channels;i++){
      info->chmuxlist[i]=oggpack_read(opb,4);
      if(info->chmuxlist[i]>=info->submaps)goto err_out;
    }
  }
  for(i=0;i<info->submaps;i++){
    info->timesubmap[i]=oggpack_read(opb,8);
    if(info->timesubmap[i]>=ci->times)goto err_out;
    info->floorsubmap[i]=oggpack_read(opb,8);
    if(info->floorsubmap[i]>=ci->floors)goto err_out;
    info->residuesubmap[i]=oggpack_read(opb,8);
    if(info->residuesubmap[i]>=ci->residues)goto err_out;
  }

  return info;

 err_out:
  mapping0_free_info(info);
  return(NULL);
}

#include "os.h"
#include "lpc.h"
#include "lsp.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"
#define VORBIS_IEEE_FLOAT32
#include "scales.h"

/* no time mapping implementation for now */
static long seq=0;
static int mapping0_forward(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state      *vd=vb->vd;
  vorbis_info           *vi=vd->vi;
  backend_lookup_state  *b=vb->vd->backend_state;
  vorbis_look_mapping0  *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0  *info=look->map;
  vorbis_info_mode      *mode=look->mode;
  vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;
  int                    n=vb->pcmend;
  int i,j;
  float *window=b->window[vb->W][vb->lW][vb->nW][mode->windowtype];

  float **pcmbundle=alloca(sizeof(float *)*vi->channels);
  int    *zerobundle=alloca(sizeof(int)*vi->channels);

  int    *nonzero=alloca(sizeof(int)*vi->channels);

  float *work=_vorbis_block_alloc(vb,n*sizeof(float));
  float newmax=vbi->ampmax;

  for(i=0;i<vi->channels;i++){
    float scale=4.f/n;
    int submap=info->chmuxlist[i];
    float ret;

    /* the following makes things clearer to *me* anyway */
    float *pcm     =vb->pcm[i]; 
    float *mdct    =pcm;
    float *logmdct =pcm+n/2;
    float *res     =pcm;
    float *codedflr=pcm+n/2;
    float *fft     =work;
    float *logfft  =work;
    float *logmax  =work;
    float *logmask =work+n/2;

    /* window the PCM data */
    for(j=0;j<n;j++)
      fft[j]=pcm[j]*=window[j];
    
    /* transform the PCM data */
    /* only MDCT right now.... */
    mdct_forward(b->transform[vb->W][0],pcm,pcm);
    for(j=0;j<n/2;j++)
      logmdct[j]=todB(mdct+j);
    
    /* FFT yields more accurate tonal estimation (not phase sensitive) */
    drft_forward(&look->fft_look,fft);
    fft[0]*=scale;
    fft[0]=todB(fft);
    for(j=1;j<n-1;j+=2){
      float temp=scale*FAST_HYPOT(fft[j],fft[j+1]);
      logfft[(j+1)>>1]=todB(&temp);
    }

    _analysis_output("fft",seq,logfft,n/2,0,0);
    _analysis_output("mdct",seq,logmdct,n/2,0,0);

    /* perform psychoacoustics; do masking */
    ret=_vp_compute_mask(look->psy_look+submap,
			 logfft, /* -> logmax */
			 logmdct,
			 logmask,
			 vbi->ampmax);
    if(ret>newmax)newmax=ret;

    _analysis_output("mask",seq,logmask,n/2,0,0);
    
    /* perform floor encoding */
    nonzero[i]=look->floor_func[submap]->
      forward(vb,look->floor_look[submap],
	      mdct,
	      logmdct,
	      logmask,
	      logmax,
	      res,
	      codedflr);

    for(j=0;j<n/2;j++)
      if(fabs(vb->pcm[i][j]>1000))
	fprintf(stderr,"%ld ",seq);
    
    _analysis_output("res",seq-vi->channels+j,vb->pcm[i],n,0,0);
    _analysis_output("codedflr",seq++,codedflr,n/2,0,1);
      
  }

  vbi->ampmax=newmax;

  /* channel coupling */
  for(i=0;i<info->coupling_steps;i++){
    if(nonzero[info->coupling_mag[i]] ||
       nonzero[info->coupling_ang[i]]){
      
      float *pcmM=vb->pcm[info->coupling_mag[i]];
      float *pcmA=vb->pcm[info->coupling_ang[i]];
      
    /*     +- 
            B
            |       A-B
     -4 -3 -2 -1  0                    
            |
      3     |     1
            |
  -+  2-----+-----2----A ++  
            |
      1     |     3
            |
      0 -1 -2 -3 -4
  B-A       |
           --

    */

      nonzero[info->coupling_mag[i]]=1; 
      nonzero[info->coupling_ang[i]]=1; 

      for(j=n/2-1;j>=0;j--){
	float A=rint(pcmM[j]);
	float B=rint(pcmA[j]);
	float mag;
	float ang;
	
	if(fabs(A)>fabs(B)){
	  mag=A;
	  if(A>0)
	    ang=A-B;
	  else
	    ang=B-A;
	}else{
	  mag=B;
	if(B>0)
	  ang=A-B;
	else
	  ang=B-A;
	}
	
	/*if(fabs(mag)<3.5f)
	  ang=rint(ang/(mag*2.f))*mag*2.f;*/
	
	if(fabs(mag)<1.5)
	ang=0;
      
	if(j>(n*3/16))
	  ang=0;
	
	if(ang>=fabs(mag*2))ang=-fabs(mag*2);
	
	pcmM[j]=mag;
	pcmA[j]=ang;
      }
    }
  }
  
  /* perform residue encoding with residue mapping; this is
     multiplexed.  All the channels belonging to one submap are
     encoded (values interleaved), then the next submap, etc */
  
  for(i=0;i<info->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(info->chmuxlist[j]==i){
	if(nonzero[j])
	  zerobundle[ch_in_bundle]=1;
	else
	  zerobundle[ch_in_bundle]=0;
	pcmbundle[ch_in_bundle++]=vb->pcm[j];
      }
    }
    
    look->residue_func[i]->forward(vb,look->residue_look[i],
				   pcmbundle,zerobundle,ch_in_bundle);
  }
  
  look->lastframe=vb->sequence;
  return(0);
}

static int mapping0_inverse(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=vi->codec_setup;
  backend_lookup_state *b=vd->backend_state;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *info=look->map;
  vorbis_info_mode     *mode=look->mode;
  int                   i,j;
  long                  n=vb->pcmend=ci->blocksizes[vb->W];

  float *window=b->window[vb->W][vb->lW][vb->nW][mode->windowtype];
  float **pcmbundle=alloca(sizeof(float *)*vi->channels);
  int    *zerobundle=alloca(sizeof(int)*vi->channels);

  int   *nonzero  =alloca(sizeof(int)*vi->channels);
  void **floormemo=alloca(sizeof(void *)*vi->channels);
  
  /* time domain information decode (note that applying the
     information would have to happen later; we'll probably add a
     function entry to the harness for that later */
  /* NOT IMPLEMENTED */

  /* recover the spectral envelope; store it in the PCM vector for now */
  for(i=0;i<vi->channels;i++){
    int submap=info->chmuxlist[i];
    floormemo[i]=look->floor_func[submap]->
      inverse1(vb,look->floor_look[submap]);
    if(floormemo[i])
      nonzero[i]=1;
    else
      nonzero[i]=0;      
    memset(vb->pcm[i],0,sizeof(float)*n/2);
  }

  /* channel coupling can 'dirty' the nonzero listing */
  for(i=0;i<info->coupling_steps;i++){
    if(nonzero[info->coupling_mag[i]] ||
       nonzero[info->coupling_ang[i]]){
      nonzero[info->coupling_mag[i]]=1; 
      nonzero[info->coupling_ang[i]]=1; 
    }
  }

  /* recover the residue into our working vectors */
  for(i=0;i<info->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(info->chmuxlist[j]==i){
	if(nonzero[j])
	  zerobundle[ch_in_bundle]=1;
	else
	  zerobundle[ch_in_bundle]=0;
	pcmbundle[ch_in_bundle++]=vb->pcm[j];
      }
    }
    
    look->residue_func[i]->inverse(vb,look->residue_look[i],
				   pcmbundle,zerobundle,ch_in_bundle);
  }

  /* channel coupling */
  for(i=info->coupling_steps-1;i>=0;i--){
    float *pcmM=vb->pcm[info->coupling_mag[i]];
    float *pcmA=vb->pcm[info->coupling_ang[i]];

    for(j=0;j<n/2;j++){
      float mag=pcmM[j];
      float ang=pcmA[j];

      if(mag>0)
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag-ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag+ang;
	}
      else
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag+ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag-ang;
	}
    }
  }

  /* compute and apply spectral envelope */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    int submap=info->chmuxlist[i];
    look->floor_func[submap]->
      inverse2(vb,look->floor_look[submap],floormemo[i],pcm);
  }

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    _analysis_output("out",seq+i,pcm,n/2,0,1);
    mdct_backward(b->transform[vb->W][0],pcm,pcm);
  }

  /* window the data */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    if(nonzero[i])
      for(j=0;j<n;j++)
	pcm[j]*=window[j];
    else
      for(j=0;j<n;j++)
	pcm[j]=0.f;
    _analysis_output("final",seq++,pcm,n,0,0);
  }
	    
  /* now apply the decoded post-window time information */
  /* NOT IMPLEMENTED */

  /* all done! */
  return(0);
}

/* export hooks */
vorbis_func_mapping mapping0_exportbundle={
  &mapping0_pack,&mapping0_unpack,&mapping0_look,&mapping0_copy_info,
  &mapping0_free_info,&mapping0_free_look,&mapping0_forward,&mapping0_inverse
};
