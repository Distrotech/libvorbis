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
 last mod: $Id: bookinternal.h,v 1.6 2000/02/23 09:24:25 xiphmont Exp $

 ********************************************************************/

#ifndef _V_INT_CODEBOOK_H_
#define _V_INT_CODEBOOK_H_

#include "vorbis/codebook.h"
#include "bitwise.h"

extern int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *b);
extern int vorbis_staticbook_unpack(oggpack_buffer *b,static_codebook *c);
extern void vorbis_staticbook_clear(static_codebook *b);

extern int vorbis_book_init_encode(codebook *dest,const static_codebook *source);
extern int vorbis_book_init_decode(codebook *dest,const static_codebook *source);
extern void vorbis_book_clear(codebook *b);

extern int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b);
extern int vorbis_book_encodev(codebook *book, double *a, oggpack_buffer *b);
extern int vorbis_book_encodevE(codebook *book, double *a, oggpack_buffer *b);
extern double vorbis_book_vE(codebook *book, double *a);
extern long vorbis_book_decode(codebook *book, oggpack_buffer *b);
extern long vorbis_book_decodev(codebook *book, double *a, oggpack_buffer *b);

#endif
