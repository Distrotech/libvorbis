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

 function: random psychoacoustics (not including preecho)
 last mod: $Id: psy.h,v 1.15 2000/10/12 03:12:53 xiphmont Exp $

 ********************************************************************/

#ifndef _V_PSY_H_
#define _V_PSY_H_
#include "smallft.h"

#ifndef EHMER_MAX
#define EHMER_MAX 56
#endif

typedef struct {
  int n;
  struct vorbis_info_psy *vi;

  float ***tonecurves;
  float **peakatt;
  float **noiseatt;

  float *ath;
  int    *octave;
  float *bark;

} vorbis_look_psy;

extern void   _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate);
extern void   _vp_psy_clear(vorbis_look_psy *p);
extern void  *_vi_psy_dup(void *source);
extern void   _vi_psy_free(vorbis_info_psy *i);
extern void   _vp_compute_mask(vorbis_look_psy *p,float *f, 
			       float *floor,
			       float *decay);
extern void _vp_apply_floor(vorbis_look_psy *p,float *f,float *flr);

#endif


