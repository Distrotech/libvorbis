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

 function: basic codebook pack/unpack/code/decode operations
 last mod: $Id: bookinternal.h,v 1.9 2000/10/12 03:12:52 xiphmont Exp $

 ********************************************************************/

#ifndef _V_INT_CODEBOOK_H_
#define _V_INT_CODEBOOK_H_

#include <ogg/ogg.h>
#include "vorbis/codebook.h"

extern int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *b);
extern int vorbis_staticbook_unpack(oggpack_buffer *b,static_codebook *c);

extern int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b);
extern int vorbis_book_errorv(codebook *book, float *a);
extern int vorbis_book_encodev(codebook *book, int best,float *a, 
			       oggpack_buffer *b);
extern int vorbis_book_encodevs(codebook *book, float *a, oggpack_buffer *b,
				int step,int stagetype);

extern long vorbis_book_decode(codebook *book, oggpack_buffer *b);
extern long vorbis_book_decodevs(codebook *book, float *a, oggpack_buffer *b,
				 int step,int stagetype);
extern long s_vorbis_book_decodevs(codebook *book, float *a, oggpack_buffer *b,
				   int step,int stagetype);

#endif
