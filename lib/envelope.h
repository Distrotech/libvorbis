/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: PCM data envelope analysis and manipulation
 last mod: $Id: envelope.h,v 1.14 2001/02/17 10:13:47 xiphmont Exp $

 ********************************************************************/

#ifndef _V_ENVELOPE_
#define _V_ENVELOPE_

#include "iir.h"
#include "smallft.h"

#define EORDER 16

typedef struct {
  int ch;
  int winlength;
  int searchstep;
  float minenergy;

  IIR_state *iir;
  float    **filtered;

  long storage;
  long current;
  long lastmark;
} envelope_lookup;

extern void _ve_envelope_init(envelope_lookup *e,vorbis_info *vi);
extern void _ve_envelope_clear(envelope_lookup *e);
extern long _ve_envelope_search(vorbis_dsp_state *v,long searchpoint);
extern void _ve_envelope_shift(envelope_lookup *e,long shift);


#endif

