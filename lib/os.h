#ifndef _OS_H
#define _OS_H
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.
 last mod: $Id: os.h,v 1.11 2000/11/06 00:07:01 xiphmont Exp $

 ********************************************************************/

#include <math.h>
#include <ogg/os_types.h>

#ifndef _V_IFDEFJAIL_H_
#define _V_IFDEFJAIL_H_

#ifndef M_PI
#define M_PI (3.1415926539)
#endif

#ifndef __GNUC__
#ifdef _WIN32
#  include <malloc.h>
#  define rint(x)   (floor((x)+0.5)) 
#endif
#endif

#ifdef _WIN32
#  define FAST_HYPOT(a, b) sqrt((a)*(a) + (b)*(b))
#else /* if not _WIN32 */
#  define FAST_HYPOT hypot
#endif

#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef USE_MEMORY_H
#include <memory.h>
#endif

#ifndef min
#  define min(x,y)  ((x)>(y)?(y):(x))
#endif

#ifndef max
#  define max(x,y)  ((x)<(y)?(y):(x))
#endif


#if defined(__i386__) && defined(__GNUC__)

/* both GCC and MSVC are kinda stupid about rounding/casting to int.
   Because of encapsulation constraints (GCC can't see inside the asm
   block and so we end up doing stupid things like a store/load that
   is collectively a noop), we do it this way */

/* we must set up the fpu before this works!! */

typedef ogg_int16_t vorbis_fpu_control;

static inline void vorbis_fpu_setround(vorbis_fpu_control *fpu){
  ogg_int16_t ret;
  ogg_int16_t temp;
  __asm__ __volatile__("fnstcw %0\n\t"
	  "movw %0,%%dx\n\t"
	  "orw $62463,%%dx\n\t"
	  "movw %%dx,%1\n\t"
	  "fldcw %1\n\t":"=m"(ret):"m"(temp): "dx");
  *fpu=ret;
}

static inline void vorbis_fpu_restore(vorbis_fpu_control fpu){
  __asm__ __volatile__("fldcw %0":: "m"(fpu));
}

/* assumes the FPU is in round mode! */
static inline int vorbis_ftoi(double f){  /* yes, double!  Otherwise,
                                             we get extra fst/fld to
                                             truncate precision */
  int i;
  __asm__("fistl %0": "=m"(i) : "t"(f));
  return(i);
}

#else

static int vorbis_ftoi(double f){
  return (int)(f+.5);
}


typedef int vorbis_fpu_control;

static inline void vorbis_fpu_setround(vorbis_fpu_control *fpu){
}

static inline void vorbis_fpu_restore(vorbis_fpu_control fpu){
}

#endif

#endif /* _OS_H */
