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

 function: linear x scale -> log x scale mappings (with bias)
 last mod: $Id: xlogmap.h,v 1.2 1999/12/30 07:26:57 xiphmont Exp $

 ********************************************************************/

#ifndef _V_XLOGMAP_H_
#define _V_XLOGMAP_H_

#include <math.h>

/*
Bias     = log_2( n / ( (2^octaves) - 1))
log_x    = log_2( linear_x + 2^Bias ) - Bias 
linear_x = 2^(Bias+log_x)-2^Bias; 
*/

#define LOG_BIAS(n,o)  (log((n)/(pow(2.,(o))-1))/log(2.))
#define LOG_X(x,b)     (log((x)+pow(2.,(b)))/log(2.)-(b))
#define LINEAR_X(x,b)  (pow(2.,((b)+(x)))-pow(2.,(b)))

#endif
