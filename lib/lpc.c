/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

  function: LPC low level routines
  author: Monty <monty@xiph.org>
  modifications by: Monty
  last modification date: Oct 11 1999

 ********************************************************************/

/* Some of these routines (autocorrelator, LPC coefficient estimator)
   are derived from code written by Jutta Degener and Carsten Bormann;
   thus we include their copyright below.  The entirety of this file
   is freely redistributable on the condition that both of these
   copyright notices are preserved without modification.  */

/* Preserved Copyright: *********************************************/

/* Copyright 1992, 1993, 1994 by Jutta Degener and Carsten Bormann,
Technische Universita"t Berlin

Any use of this software is permitted provided that this notice is not
removed and that neither the authors nor the Technische Universita"t
Berlin are deemed to have made any representations as to the
suitability of this software for any purpose nor are held responsible
for any defects of this software. THERE IS ABSOLUTELY NO WARRANTY FOR
THIS SOFTWARE.

As a matter of courtesy, the authors request to be informed about uses
this software has found, about bugs in this software, and about any
improvements that may be of general interest.

Berlin, 28.11.1994
Jutta Degener
Carsten Bormann

*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "smallft.h"
#include "lpc.h"
#include "xlogmap.h"

/* This is pared down for Vorbis where we only use LPC to encode
   spectral envelope curves.  Thus we only are interested in
   generating the coefficients and recovering the curve from the
   coefficients.  Autocorrelation LPC coeff generation algorithm
   invented by N. Levinson in 1947, modified by J. Durbin in 1959. */

/* Input : n element envelope curve
   Output: m lpc coefficients, excitation energy */

double vorbis_gen_lpc(double *curve,double *lpc,lpc_lookup *l){
  int n=l->ln;
  int m=l->m;
  double aut[m+1],work[n+n],error;
  double fscale=1./n;
  int i,j;
  
  /* input is a real curve. make it complex-real */
  /* This mixes phase, but the LPC generation doesn't care. */
  for(i=0;i<n;i++){
    work[i*2]=curve[i]*fscale;
    work[i*2+1]=0;
  }

  n*=2;
  drft_backward(&l->fft,work);

  /* The autocorrelation will not be circular.  Shift, else we lose
     most of the power in the edges. */
  
  for(i=0,j=n/2;i<n/2;){
    double temp=work[i];
    work[i++]=work[j];
    work[j++]=temp;
  }
  
  /* autocorrelation, p+1 lag coefficients */

  j=m+1;
  while(j--){
    double d=0;
    for(i=j;i<n;i++)d+=work[i]*work[i-j];
    aut[j]=d;
  }

  /* Generate lpc coefficients from autocorr values */

  error=aut[0];
  if(error==0){
    memset(lpc,0,m*sizeof(double));
    return 0;
  }
  
  for(i=0;i<m;i++){
    double r=-aut[i+1];

    /* Sum up this iteration's reflection coefficient; note that in
       Vorbis we don't save it.  If anyone wants to recycle this code
       and needs reflection coefficients, save the results of 'r' from
       each iteration. */

    for(j=0;j<i;j++)r-=lpc[j]*aut[i-j];
    r/=error; 

    /* Update LPC coefficients and total error */

    lpc[i]=r;
    for(j=0;j<i/2;j++){
      double tmp=lpc[j];
      lpc[j]+=r*lpc[i-1-j];
      lpc[i-1-j]+=r*tmp;
    }
    if(i%2)lpc[j]+=lpc[j]*r;
    
    error*=1.0-r*r;
  }

  /* we need the error value to know how big an impulse to hit the
     filter with later */
  
  return error;
}

/* On top of this basic LPC infrastructure we introduce two modifications:

   1) Filter generation is limited in the resolution of features it
   can represent (this is more obvious when the filter is looked at as
   a set of LSP coefficients).  Human perception of the audio spectrum
   is logarithmic not only in amplitude, but also frequency.  Because
   the high frequency features we'll need to encode will be broader
   than the low frequency features, filter generation will be
   dominated by higher frequencies (when most of the energy is in the
   lowest frequencies, and greatest perceived resolution is in the
   midrange).  To avoid this effect, Vorbis encodes the frequency
   spectrum with a biased log frequency scale. The intent is to
   roughly equalize the sizes of the octaves (see xlogmap.h)

   2) When we change the frequency scale, we also change the
   (apparent) relative energies of the bands; that is, on a log scale
   covering 5 octaves, the highest octave goes from being represented
   in half the bins, to only 1/32 of the bins.  If the amplitudes
   remain the same, we have divided the energy represented by the
   highest octave by 16 (as far as Levinson-Durbin is concerned).
   This will seriously skew filter generation, which bases calculation
   on the mean square error with respect to energy.  Thus, Vorbis
   normalizes the amplitudes of the log spectrum frequencies to keep
   the relative octave energies correct. */

/* n == size of vector to be used for filter, m == order of filter,
   oct == octaves in normalized scale, encode_p == encode (1) or
   decode (0) */

void lpc_init(lpc_lookup *l,int n, int mapped, int m, int oct, int encode_p){
  double bias=LOG_BIAS(n,oct);
  double scale=(float)mapped/(float)oct; /* where n==mapped */    
  int i;

  memset(l,0,sizeof(lpc_lookup));

  l->n=n;
  l->ln=mapped;
  l->m=m;
  l->iscale=malloc(n*sizeof(int));
  l->norm=malloc(n*sizeof(double));

  for(i=0;i<n;i++){
    /* how much 'real estate' in the log domain does the bin in the
       linear domain represent? */
    double logA=LOG_X(i-.5,bias);
    double logB=LOG_X(i+.5,bias);
    l->norm[i]=logB-logA;  /* this much */
  }

  /* the scale is encode/decode specific for algebraic simplicity */

  if(encode_p){
    /* encode */
    l->bscale=malloc(n*sizeof(int));
    l->escale=malloc(n*sizeof(double));

    for(i=0;i<n;i++){
      l->escale[i]=LINEAR_X(i/scale,bias);
      l->bscale[i]=rint(LOG_X(i,bias)*scale);
    }   

  }
  /* decode; encode may use this too */
  
  drft_init(&l->fft,mapped*2);
  {
    double w=1./oct*M_PI;
    for(i=0;i<n;i++){
      l->iscale[i]=rint(LOG_X(i,bias)/oct*mapped);
      if(l->iscale[i]>=l->ln)l->iscale[i]=l->ln-1;
    }
  }
}

void lpc_clear(lpc_lookup *l){
  if(l){
    if(l->bscale)free(l->bscale);
    if(l->escale)free(l->escale);
    drft_clear(&l->fft);
    free(l->iscale);
    free(l->norm);
  }
}


/* less efficient than the decode side (written for clarity).  We're
   not bottlenecked here anyway */

double vorbis_curve_to_lpc(double *curve,double *lpc,lpc_lookup *l){
  /* map the input curve to a log curve for encoding */

  /* for clarity, mapped and n are both represented although setting
     'em equal is a decent rule of thumb. The below must be reworked
     slightly if mapped != n */
  
  int mapped=l->ln;
  double work[mapped];
  int i;

  /* fairly correct for low frequencies, naieve for high frequencies
     (suffers from undersampling) */
  for(i=0;i<mapped;i++){
    double lin=l->escale[i];
    int a=floor(lin);
    int b=ceil(lin);
    double del=lin-floor(lin);

    work[i]=(curve[a]/l->norm[a]*(1.-del)+
	     curve[b]/l->norm[b]*del);      

  }

  return vorbis_gen_lpc(work,lpc,l);
}


/* One can do this the long way by generating the transfer function in
   the time domain and taking the forward FFT of the result.  The
   results from direct calculation are cleaner and faster. If one
   looks at the below in the context of the calling function, there's
   lots of redundant trig, but at least it's clear */

/* This version does a linear curve generation and then later
   interpolates the log curve from the linear curve.  This could stand
   optimization; it could both be more precise as well as not compute
   quite a few unused values */

static void _vlpc_de_helper(double *curve,double *lpc,double amp,
			    lpc_lookup *l){
  int i;
  memset(curve,0,sizeof(double)*l->ln*2);
  
  for(i=0;i<l->m;i++){
    curve[i*2+1]=lpc[i]/4/amp;
    curve[i*2+2]=-lpc[i]/4/amp;
  }

  drft_backward(&l->fft,curve); /* reappropriated ;-) */

  {
    int l2=l->ln*2;
    double unit=1./amp;
    curve[0]=(1./(curve[0]+unit));
    for(i=1;i<l->ln;i++){
      double real=(curve[i]+curve[l2-i]);
      double imag=(curve[i]-curve[l2-i]);
      curve[i]=(1./hypot(real+unit,imag));
    }
  }
}
  

/* generate the whole freq response curve on an LPC IIR filter */

void vorbis_lpc_to_curve(double *curve,double *lpc,double amp,lpc_lookup *l){
  double lcurve[l->ln*2];
  int i;

  _vlpc_de_helper(lcurve,lpc,amp,l);

  for(i=0;i<l->n;i++)
    curve[i]=lcurve[l->iscale[i]]*l->norm[i];
}

void vorbis_lpc_apply(double *residue,double *lpc,double amp,lpc_lookup *l){
  double lcurve[l->ln*2];
  int i;

  _vlpc_de_helper(lcurve,lpc,amp,l);

  for(i=0;i<l->n;i++)
    residue[i]*=lcurve[l->iscale[i]]*l->norm[i];
}

