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
 last mod: $Id: psy.h,v 1.6 2000/01/22 13:28:28 xiphmont Exp $

 ********************************************************************/

#ifndef _V_PSY_H_
#define _V_PSY_H_

typedef struct {
  int n;
  struct vorbis_info_psy *vi;

  double *maskthresh;
  double *barknum;

} psy_lookup;

extern void   _vp_psy_init(psy_lookup *p,vorbis_info_psy *vi,int n,long rate);
extern void   _vp_psy_clear(psy_lookup *p);
extern void  *_vi_psy_dup(void *source);
extern void   _vi_psy_free(void *i);

extern void   _vp_mask_floor(psy_lookup *p,double *pcm,
			     double *mask,double *floor);

#endif


