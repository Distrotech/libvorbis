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

 function: toplevel residue templates for 32/44.1/48kHz
 last mod: $Id: residue_44.h,v 1.2 2001/12/12 09:45:55 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "backends.h"

static bitrate_manager_info _bm_44_default={
  /* progressive coding and bitrate controls */
  2.,.5,
  2.,       0,           0,  
            0,           0,
           
  4.0, 0.,  -1.,              .05, 
            -.05,             .05,
  3.5,5.0,
  -10.f,+2.f
};

/***** residue backends *********************************************/

/* the books used depend on stereo-or-not, but the residue setup below
   can apply to coupled or not.  These templates are for a first pass;
   a last pass is mechanically added in vorbisenc for residue backfill
   at 1/3 and 1/9, as well as an optional middle pass for stereo
   backfill */

/*     0   1   2   4  26   1   2   4  26   +      
           0   0   0   0         

       0   1   2   3   4   5   6   7   8   9
   1                   .               .   .
   2                   .               .   .
   4       .   .   .       .   .   .       .
 
       0   4   4   4   3   4   4   4   3   7 */
static vorbis_info_residue0 _residue_44_low={
  0,-1, -1, 8,-1,
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  1.5,  2.5,  4.5, 26.5,  1.5,  4.5},
  {0},
  {  99,   -1,   -1,   -1,   -1,   99,   99}
};

/*     0   1   2   4   1   2   4  16  42   +      
           0   0   0            

       0   1   2   3   4   5   6   7   8   9
   1                               .   .   .
   2                               .   .   .
   4       .   .   .   .   .   .           .
 
       0   4   4   4   4   4   4   3   3   7 */
static vorbis_info_residue0 _residue_44_mid={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  1.5,  1.5,  2.5,  2.5,  4.5,  4.5, 16.5, 42.5},
  {0},
  {  99,   -1,   99,   -1,   99,   -1,   99,   99,   99}
};


/*     0   4  42   1   2   4   8  16  56   +      
           0   0   0            

       0   1   2   3   4   5   6   7   8   9
   1           .                   .   .   .
   2           .                   .   .   .
   4       .       .   .   .   .           .
 
       0   4   3   4   4   4   4   3   3   7 */
static vorbis_info_residue0 _residue_44_high={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  4.5, 42.5,  1.5,  2.5,  4.5,  8.5, 16.5, 56.5},
  {0},
  {  99,   -1,   -1,   99,   99,   99,   99,   99,   99}
};

#include "books/coupled/_44c0_short.vqh"
#include "books/coupled/_44c0_long.vqh"

#include "books/coupled/_44c0_s0_p1_0.vqh"
#include "books/coupled/_44c0_s0_p2_0.vqh"
#include "books/coupled/_44c0_s0_p3_0.vqh"
#include "books/coupled/_44c0_s0_p4_0.vqh"
#include "books/coupled/_44c0_s0_p4_1.vqh"
#include "books/coupled/_44c0_s1_p5_0.vqh"
#include "books/coupled/_44c0_s2_p6_0.vqh"
#include "books/coupled/_44c0_s4_p7_0.vqh"
#include "books/coupled/_44c0_s4_p7_1.vqh"
#include "books/coupled/_44c0_s4_p7_2.vqh"

#include "books/coupled/_44c1_short.vqh"
#include "books/coupled/_44c1_long.vqh"

#include "books/coupled/_44c1_s0_p1_0.vqh"
#include "books/coupled/_44c1_s0_p2_0.vqh"
#include "books/coupled/_44c1_s0_p3_0.vqh"
#include "books/coupled/_44c1_s0_p4_0.vqh"
#include "books/coupled/_44c1_s0_p4_1.vqh"
#include "books/coupled/_44c1_s1_p5_0.vqh"
#include "books/coupled/_44c1_s2_p6_0.vqh"
#include "books/coupled/_44c1_s3_p7_0.vqh"
#include "books/coupled/_44c1_s3_p7_1.vqh"
#include "books/coupled/_44c1_s3_p7_2.vqh"

#include "books/coupled/_44c2_short.vqh"
#include "books/coupled/_44c2_long.vqh"

#include "books/coupled/_44c2_s0_p1_0.vqh"
#include "books/coupled/_44c2_s0_p2_0.vqh"
#include "books/coupled/_44c2_s0_p3_0.vqh"
#include "books/coupled/_44c2_s0_p4_0.vqh"
#include "books/coupled/_44c2_s0_p4_1.vqh"
#include "books/coupled/_44c2_s1_p5_0.vqh"
#include "books/coupled/_44c2_s2_p6_0.vqh"
#include "books/coupled/_44c2_s2_p7_0.vqh"
#include "books/coupled/_44c2_s2_p7_1.vqh"
#include "books/coupled/_44c2_s2_p7_2.vqh"

#include "books/coupled/_44c3_short.vqh"
#include "books/coupled/_44c3_long.vqh"

#include "books/coupled/_44c3_s0_p1_0.vqh"
#include "books/coupled/_44c3_s0_p2_0.vqh"
#include "books/coupled/_44c3_s0_p3_0.vqh"
#include "books/coupled/_44c3_s0_p4_0.vqh"
#include "books/coupled/_44c3_s0_p4_1.vqh"
#include "books/coupled/_44c3_s1_p5_0.vqh"
#include "books/coupled/_44c3_s2_p6_0.vqh"
#include "books/coupled/_44c3_s2_p7_0.vqh"
#include "books/coupled/_44c3_s2_p7_1.vqh"
#include "books/coupled/_44c3_s2_p7_2.vqh"

#include "books/coupled/_44c4_short.vqh"
#include "books/coupled/_44c4_long.vqh"

#include "books/coupled/_44c4_s0_p1_0.vqh"
#include "books/coupled/_44c4_s1_p2_0.vqh"
#include "books/coupled/_44c4_s0_p3_0.vqh"
#include "books/coupled/_44c4_s1_p4_0.vqh"
#include "books/coupled/_44c4_s0_p5_0.vqh"
#include "books/coupled/_44c4_s1_p6_0.vqh"
#include "books/coupled/_44c4_s1_p7_0.vqh"
#include "books/coupled/_44c4_s1_p7_1.vqh"
#include "books/coupled/_44c4_s1_p8_0.vqh"
#include "books/coupled/_44c4_s1_p8_1.vqh"
#include "books/coupled/_44c4_s1_p9_0.vqh"
#include "books/coupled/_44c4_s1_p9_1.vqh"
#include "books/coupled/_44c4_s1_p9_2.vqh"

/* residue backfill is entered in the template array as if stereo
   backfill is not in use.  It's up to vorbisenc to make the
   appropriate index adjustment */
static vorbis_residue_template _residue_template_44_stereo[11]={
  /* mode 0; 64-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c0_short,&_huff_book__44c0_long},
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     {{0},{0,0,&_44c0_s0_p1_0},{0,0,&_44c0_s0_p2_0},{0,0,&_44c0_s0_p3_0},
      {&_44c0_s0_p4_0,&_44c0_s0_p4_1},{0,0,&_44c0_s1_p5_0},{0,0,&_44c0_s2_p6_0},
      {&_44c0_s4_p7_0,&_44c0_s4_p7_1,&_44c0_s4_p7_2}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     //{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},/* 24dB (16.5) stereo */
   },
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     //{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
     // {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
     // {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
     // {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
     // {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },

  /* mode 1; 80-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c1_short,&_huff_book__44c1_long},
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0},{0,0,&_44c1_s0_p1_0},{0,0,&_44c1_s0_p2_0},{0,0,&_44c1_s0_p3_0},
      {&_44c1_s0_p4_0,&_44c1_s0_p4_1},{0,0,&_44c1_s1_p5_0},{0,0,&_44c1_s2_p6_0},
      {&_44c1_s3_p7_0,&_44c1_s3_p7_1,&_44c1_s3_p7_2}}, /* 24dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     //{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},/* 24dB (16.5) stereo */
   },
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     //{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
     // {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
     // {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
     // {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
     // {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },

  /* mode 2; 96-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c2_short,&_huff_book__44c2_long},
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0},{0,0,&_44c2_s0_p1_0},{0,0,&_44c2_s0_p2_0},{0,0,&_44c2_s0_p3_0},
      {&_44c2_s0_p4_0,&_44c2_s0_p4_1},{0,0,&_44c2_s1_p5_0},{0,0,&_44c2_s2_p6_0},
      {&_44c2_s2_p7_0,&_44c2_s2_p7_1,&_44c2_s2_p7_2}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     //{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},/* 24dB (16.5) stereo */
   },
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     //{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
     // {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
     // {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
     // {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
     // {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },

  /* mode 3; 112-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c3_short,&_huff_book__44c3_long},
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0},{0,0,&_44c3_s0_p1_0},{0,0,&_44c3_s0_p2_0},{0,0,&_44c3_s0_p3_0},
      {&_44c3_s0_p4_0,&_44c3_s0_p4_1},{0,0,&_44c3_s1_p5_0},{0,0,&_44c3_s2_p6_0},
      {&_44c3_s2_p7_0,&_44c3_s2_p7_1,&_44c3_s2_p7_2}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     //{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},/* 24dB (16.5) stereo */
   },
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     //{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
     // {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
     // {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
     // {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
     // {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },

  /* mode 4; 128-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c4_short,&_huff_book__44c4_long},
   { {{0}}, /* lossless stereo */
     {{0},{0,0,&_44c4_s0_p1_0},{0,0,&_44c4_s1_p2_0},{0,0,&_44c4_s0_p3_0},
      {0,0,&_44c4_s1_p4_0},{0,0,&_44c4_s0_p5_0},{0,0,&_44c4_s1_p6_0},
      {&_44c4_s1_p7_0,&_44c4_s1_p7_1},{&_44c4_s1_p8_0,&_44c4_s1_p8_1},
      {&_44c4_s1_p9_0,&_44c4_s1_p9_1,&_44c4_s1_p9_2}}, 
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     //{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},/* 24dB (16.5) stereo */
   },
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     //{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
     // {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
     // {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
     // {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
     // {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  }

};















