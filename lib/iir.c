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

  function: Direct Form I, II IIR filters, plus some specializations
  last mod: $Id: iir.c,v 1.5 2000/11/14 00:05:31 xiphmont Exp $

 ********************************************************************/

/* LPC is actually a degenerate case of form I/II filters, but we need
   both */

#include <ogg/ogg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iir.h"

void IIR_init(IIR_state *s,int stages,float gain, float *A, float *B){
  memset(s,0,sizeof(IIR_state));
  s->stages=stages;
  s->gain=gain;
  s->coeff_A=_ogg_malloc(stages*sizeof(float));
  s->coeff_B=_ogg_malloc((stages+1)*sizeof(float));
  s->z_A=_ogg_calloc(stages*2,sizeof(float));

  memcpy(s->coeff_A,A,stages*sizeof(float));
  memcpy(s->coeff_B,B,(stages+1)*sizeof(float));
}

void IIR_clear(IIR_state *s){
  if(s){
    _ogg_free(s->coeff_A);
    _ogg_free(s->coeff_B);
    _ogg_free(s->z_A);
    memset(s,0,sizeof(IIR_state));
  }
}

void IIR_reset(IIR_state *s){
  memset(s->z_A,0,sizeof(float)*s->stages*2);
}

float IIR_filter(IIR_state *s,float in){
  int stages=s->stages,i;
  float newA;
  float newB=0;
  float *zA=s->z_A+s->ring;

  newA=in/=s->gain;
  for(i=0;i<stages;i++){
    newA+= s->coeff_A[i] * zA[i];
    newB+= s->coeff_B[i] * zA[i];
  }
  newB+=newA*s->coeff_B[stages];

  zA[0]=zA[stages]=newA;
  if(++s->ring>=stages)s->ring=0;

  return(newB);
}

/* this assumes the symmetrical structure of the feed-forward stage of
   a Chebyshev bandpass to save multiplies */
float IIR_filter_ChebBand(IIR_state *s,float in){
  int stages=s->stages,i;
  float newA;
  float newB=0;
  float *zA=s->z_A+s->ring;

  newA=in/=s->gain;

  newA+= s->coeff_A[0] * zA[0];
  for(i=1;i<(stages>>1);i++){
    newA+= s->coeff_A[i] * zA[i];
    newB+= s->coeff_B[i] * (zA[i]-zA[stages-i]);
  }
  newB+= s->coeff_B[i] * zA[i];
  for(;i<stages;i++)
    newA+= s->coeff_A[i] * zA[i];

  newB+= newA-zA[0];

  zA[0]=zA[stages]=newA;
  if(++s->ring>=stages)s->ring=0;

  return(newB);
}

#ifdef _V_SELFTEST

/* z^-stage, z^-stage+1... */
static float cheb_bandpass_B[]={-1.,0.,5.,0.,-10.,0.,10.,0.,-5.,0.,1};
static float cheb_bandpass_A[]={-0.6665900311,
				  1.0070146601,
				 -3.1262875409,
			 	  3.5017171569,
				 -6.2779211945,
				  5.2966481740,
				 -6.7570216587,
				  4.0760335768,
				 -3.9134284363,
				  1.3997338886};

static float data[128]={  
  0.0426331,
  0.0384521,
  0.0345764,
  0.0346069,
  0.0314636,
  0.0310059,
  0.0318604,
  0.0336304,
  0.036438,
  0.0348511,
  0.0354919,
  0.0343628,
  0.0325623,
  0.0318909,
  0.0263367,
  0.0225525,
  0.0195618,
  0.0160828,
  0.0168762,
  0.0145569,
  0.0126343,
  0.0127258,
  0.00820923,
  0.00787354,
  0.00558472,
  0.00204468,
  3.05176e-05,
  -0.00357056,
  -0.00570679,
  -0.00991821,
  -0.0101013,
  -0.00881958,
  -0.0108948,
  -0.0110168,
  -0.0119324,
  -0.0161438,
  -0.0194702,
  -0.0229187,
  -0.0260315,
  -0.0282288,
  -0.0306091,
  -0.0330505,
  -0.0364685,
  -0.0385742,
  -0.0428772,
  -0.043457,
  -0.0425415,
  -0.0462341,
  -0.0467529,
  -0.0489807,
  -0.0520325,
  -0.0558167,
  -0.0596924,
  -0.0591431,
  -0.0612793,
  -0.0618591,
  -0.0615845,
  -0.0634155,
  -0.0639648,
  -0.0683594,
  -0.0718079,
  -0.0729675,
  -0.0791931,
  -0.0860901,
  -0.0885315,
  -0.088623,
  -0.089386,
  -0.0899353,
  -0.0886841,
  -0.0910645,
  -0.0948181,
  -0.0919495,
  -0.0891418,
  -0.0916443,
  -0.096344,
  -0.100464,
  -0.105499,
  -0.108612,
  -0.112213,
  -0.117676,
  -0.120911,
  -0.124329,
  -0.122162,
  -0.120605,
  -0.12326,
  -0.12619,
  -0.128998,
  -0.13205,
  -0.134247,
  -0.137939,
  -0.143555,
  -0.14389,
  -0.14859,
  -0.153717,
  -0.159851,
  -0.164551,
  -0.162811,
  -0.164276,
  -0.156952,
  -0.140564,
  -0.123291,
  -0.10321,
  -0.0827637,
  -0.0652466,
  -0.053772,
  -0.0509949,
  -0.0577698,
  -0.0818176,
  -0.114929,
  -0.148895,
  -0.181122,
  -0.200714,
  -0.21048,
  -0.203644,
  -0.179413,
  -0.145325,
  -0.104492,
  -0.0658264,
  -0.0332031,
  -0.0106201,
  -0.00363159,
  -0.00909424,
  -0.0244141,
  -0.0422058,
  -0.0537415,
  -0.0610046,
  -0.0609741,
  -0.0547791};

/* comparison test code from http://www-users.cs.york.ac.uk/~fisher/mkfilter/
   (the above page kicks ass, BTW)*/

#define NZEROS 10
#define NPOLES 10
#define GAIN   4.599477515e+02

static float xv[NZEROS+1], yv[NPOLES+1];

static float filterloop(float next){ 
  xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; 
  xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; xv[8] = xv[9]; xv[9] = xv[10]; 
  xv[10] = next / GAIN;
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; 
  yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; yv[8] = yv[9]; yv[9] = yv[10]; 
  yv[10] =   (xv[10] - xv[0]) + 5 * (xv[2] - xv[8]) + 10 * (xv[6] - xv[4])
    + ( -0.6665900311 * yv[0]) + (  1.0070146601 * yv[1])
    + ( -3.1262875409 * yv[2]) + (  3.5017171569 * yv[3])
    + ( -6.2779211945 * yv[4]) + (  5.2966481740 * yv[5])
    + ( -6.7570216587 * yv[6]) + (  4.0760335768 * yv[7])
    + ( -3.9134284363 * yv[8]) + (  1.3997338886 * yv[9]);
  return(yv[10]);
}

#include <stdio.h>
int main(){

  /* run the pregenerated Chebyshev filter, then our own distillation
     through the generic and specialized code */
  float *work=_ogg_malloc(128*sizeof(float));
  IIR_state iir;
  int i;

  for(i=0;i<128;i++)work[i]=filterloop(data[i]);
  {
    FILE *out=fopen("IIR_ref.m","w");
    for(i=0;i<128;i++)fprintf(out,"%g\n",work[i]);
    fclose(out);
  }

  IIR_init(&iir,NPOLES,GAIN,cheb_bandpass_A,cheb_bandpass_B);
  for(i=0;i<128;i++)work[i]=IIR_filter(&iir,data[i]);
  {
    FILE *out=fopen("IIR_gen.m","w");
    for(i=0;i<128;i++)fprintf(out,"%g\n",work[i]);
    fclose(out);
  }
  IIR_clear(&iir);  

  IIR_init(&iir,NPOLES,GAIN,cheb_bandpass_A,cheb_bandpass_B);
  for(i=0;i<128;i++)work[i]=IIR_filter_ChebBand(&iir,data[i]);
  {
    FILE *out=fopen("IIR_cheb.m","w");
    for(i=0;i<128;i++)fprintf(out,"%g\n",work[i]);
    fclose(out);
  }
  IIR_clear(&iir);  

  return(0);
}

#endif
