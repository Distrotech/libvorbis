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

 function: bark scale utility
 last mod: $Id: barkmel.c,v 1.2 2000/11/06 00:07:00 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include "scales.h"
int main(){
  int i;
  double rate;
  for(i=64;i<32000;i*=2){
    rate=48000.;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=44100.;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=32000.;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=22050.;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=16000.;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=11025.;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=8000.;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));


  }
  for(i=0;i<28;i++){
    fprintf(stderr,"bark=%d %gHz\n",
	    i,fromBARK(i));
  }
  return(0);
}

