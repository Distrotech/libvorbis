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

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.54 2001/09/11 02:42:34 segher Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

#define NEGINF -9999.f

/* Why Bark scale for encoding but not masking computation? Because
   masking has a strong harmonic dependency */

vorbis_look_psy_global *_vp_global_look(vorbis_info *vi){
  int i,j;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=ci->psy_g_param;
  vorbis_look_psy_global *look=_ogg_calloc(1,sizeof(vorbis_look_psy_global));

  int shiftoc=rint(log(gi->eighth_octave_lines*8)/log(2))-1;
  look->decaylines=toOC(96000.f)*(1<<(shiftoc+1))+.5f; /* max sample
							  rate of
							  192000kHz
							  for now */
  look->decay=_ogg_calloc(vi->channels,sizeof(float *));
  for(i=0;i<vi->channels;i++){
    look->decay[i]=_ogg_calloc(look->decaylines,sizeof(float));
    for(j=0;j<look->decaylines;j++)
      look->decay[i][j]=-9999.;
  }
  look->channels=vi->channels;

  look->ampmax=-9999.;
  look->gi=gi;
  return(look);
}

void _vp_global_free(vorbis_look_psy_global *look){
  int i;
  if(look->decay){
    for(i=0;i<look->channels;i++)
      _ogg_free(look->decay[i]);
    _ogg_free(look->decay);
  }
  memset(look,0,sizeof(vorbis_look_psy_global));
  _ogg_free(look);
}

void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_psy));
    _ogg_free(i);
  }
}

vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i){
  vorbis_info_psy *ret=_ogg_malloc(sizeof(vorbis_info_psy));
  memcpy(ret,i,sizeof(vorbis_info_psy));
  return(ret);
}

/* Set up decibel threshold slopes on a Bark frequency scale */
/* ATH is the only bit left on a Bark scale.  No reason to change it
   right now */
static void set_curve(float *ref,float *c,int n, float crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK(i+1)*2*n/crate);
    float base=ref[i];
    if(j<endpos){
      float delta=(ref[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
	c[j]=base;
	base+=delta;
      }
    }
  }
}

static void min_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(float *c,float att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

static void interp_curve(float *c,float *c1,float *c2,float del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=c2[i]*del+c1[i]*(1.f-del);
}

extern int analysis_noisy;
static void setup_curve(float **c,
			int band,
			float *curveatt_dB){
  int i,j;
  float ath[EHMER_MAX];
  float tempc[P_LEVELS][EHMER_MAX];
  float *ATH=ATH_Bark_dB_lspconservative; /* just for limiting here */

  memcpy(c[0]+2,c[4]+2,sizeof(float)*EHMER_MAX);
  memcpy(c[2]+2,c[4]+2,sizeof(float)*EHMER_MAX);

  /* we add back in the ATH to avoid low level curves falling off to
     -infinity and unnecessarily cutting off high level curves in the
     curve limiting (last step).  But again, remember... a half-band's
     settings must be valid over the whole band, and it's better to
     mask too little than too much, so be pessimistical. */

  for(i=0;i<EHMER_MAX;i++){
    float oc_min=band*.5+(i-EHMER_OFFSET)*.125;
    float oc_max=band*.5+(i-EHMER_OFFSET+1)*.125;
    float bark=toBARK(fromOC(oc_min));
    int ibark=floor(bark);
    float del=bark-ibark;
    float ath_min,ath_max;

    if(ibark<26)
      ath_min=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_min=ATH[25];

    bark=toBARK(fromOC(oc_max));
    ibark=floor(bark);
    del=bark-ibark;

    if(ibark<26)
      ath_max=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_max=ATH[25];

    ath[i]=min(ath_min,ath_max);
  }

  /* The c array comes in as dB curves at 20 40 60 80 100 dB.
     interpolate intermediate dB curves */
  for(i=1;i<P_LEVELS;i+=2){
    interp_curve(c[i]+2,c[i-1]+2,c[i+1]+2,.5);
  }

  /* normalize curves so the driving amplitude is 0dB */
  /* make temp curves with the ATH overlayed */
  for(i=0;i<P_LEVELS;i++){
    attenuate_curve(c[i]+2,curveatt_dB[i]);
    memcpy(tempc[i],ath,EHMER_MAX*sizeof(float));
    attenuate_curve(tempc[i],-i*10.f);
    max_curve(tempc[i],c[i]+2);
  }

  /* Now limit the louder curves.

     the idea is this: We don't know what the playback attenuation
     will be; 0dB SL moves every time the user twiddles the volume
     knob. So that means we have to use a single 'most pessimal' curve
     for all masking amplitudes, right?  Wrong.  The *loudest* sound
     can be in (we assume) a range of ...+100dB] SL.  However, sounds
     20dB down will be in a range ...+80], 40dB down is from ...+60],
     etc... */

  for(j=1;j<P_LEVELS;j++){
    min_curve(tempc[j],tempc[j-1]);
    min_curve(c[j]+2,tempc[j]);
  }

  /* add fenceposts */
  for(j=0;j<P_LEVELS;j++){

    for(i=0;i<EHMER_OFFSET;i++)
      if(c[j][i+2]>-200.f)break;  
    c[j][0]=i;

    for(i=EHMER_MAX-1;i>EHMER_OFFSET+1;i--)
      if(c[j][i+2]>-200.f)
	break;
    c[j][1]=i;

  }
}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
		  vorbis_info_psy_global *gi,int n,long rate){
  long i,j,k,lo=-99,hi=0;
  long maxoc;
  memset(p,0,sizeof(vorbis_look_psy));


  p->eighth_octave_lines=gi->eighth_octave_lines;
  p->shiftoc=rint(log(gi->eighth_octave_lines*8)/log(2))-1;

  p->firstoc=toOC(.25f*rate/n)*(1<<(p->shiftoc+1))-gi->eighth_octave_lines;
  maxoc=toOC((n*.5f-.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;
  p->total_octave_lines=maxoc-p->firstoc+1;

  if(vi->ath)
    p->ath=_ogg_malloc(n*sizeof(float));
  p->octave=_ogg_malloc(n*sizeof(long));
  p->bark=_ogg_malloc(n*sizeof(unsigned long));
  p->vi=vi;
  p->n=n;
  p->rate=rate;

  /* set up the lookups for a given blocksize and sample rate */
  if(vi->ath)
    set_curve(vi->ath, p->ath,n,rate);
  for(i=0;i<n;i++){
    float bark=toBARK(rate/(2*n)*i); 

    for(;lo+vi->noisewindowlomin<i && 
	  toBARK(rate/(2*n)*lo)<(bark-vi->noisewindowlo);lo++);
    
    for(;hi<n && (hi<i+vi->noisewindowhimin ||
	  toBARK(rate/(2*n)*hi)<(bark+vi->noisewindowhi));hi++);
    
    p->bark[i]=(lo<<16)+hi;

  }

  for(i=0;i<n;i++)
    p->octave[i]=toOC((i*.5f+.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;

  p->tonecurves=_ogg_malloc(P_BANDS*sizeof(float **));
  p->noisethresh=_ogg_malloc(n*sizeof(float));
  p->noiseoffset=_ogg_malloc(n*sizeof(float));
  for(i=0;i<P_BANDS;i++)
    p->tonecurves[i]=_ogg_malloc(P_LEVELS*sizeof(float *));
  
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      p->tonecurves[i][j]=_ogg_malloc((EHMER_MAX+2)*sizeof(float));
  

  /* OK, yeah, this was a silly way to do it */
  memcpy(p->tonecurves[0][4]+2,tone_125_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][6]+2,tone_125_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][8]+2,tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][10]+2,tone_125_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[2][4]+2,tone_125_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][6]+2,tone_125_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][8]+2,tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][10]+2,tone_125_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[4][4]+2,tone_250_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][6]+2,tone_250_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][8]+2,tone_250_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][10]+2,tone_250_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[6][4]+2,tone_500_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][6]+2,tone_500_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][8]+2,tone_500_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][10]+2,tone_500_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[8][4]+2,tone_1000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][6]+2,tone_1000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][8]+2,tone_1000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][10]+2,tone_1000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[10][4]+2,tone_2000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][6]+2,tone_2000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][8]+2,tone_2000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][10]+2,tone_2000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[12][4]+2,tone_4000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][6]+2,tone_4000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][8]+2,tone_4000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][10]+2,tone_4000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[14][4]+2,tone_8000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][6]+2,tone_8000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][8]+2,tone_8000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][10]+2,tone_8000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[16][4]+2,tone_8000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][6]+2,tone_8000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][8]+2,tone_8000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][10]+2,tone_8000_100dB_SL,sizeof(float)*EHMER_MAX);

  /* value limit the tonal masking curves; the peakatt not only
     optionally specifies maximum dynamic depth, but also [always]
     limits the masking curves to a minimum depth */
  for(i=0;i<P_BANDS;i+=2)
    for(j=4;j<P_LEVELS;j+=2)
      for(k=2;k<EHMER_MAX+2;k++)
	p->tonecurves[i][j][k]+=vi->tone_masteratt;

  /* interpolate curves between */
  for(i=1;i<P_BANDS;i+=2)
    for(j=4;j<P_LEVELS;j+=2){
      memcpy(p->tonecurves[i][j]+2,p->tonecurves[i-1][j]+2,EHMER_MAX*sizeof(float));
      /*interp_curve(p->tonecurves[i][j],
		   p->tonecurves[i-1][j],
		   p->tonecurves[i+1][j],.5);*/
      min_curve(p->tonecurves[i][j]+2,p->tonecurves[i+1][j]+2);
    }

  /* set up the final curves */
  for(i=0;i<P_BANDS;i++)
    setup_curve(p->tonecurves[i],i,vi->toneatt->block[i]);

  if(vi->curvelimitp){
    /* value limit the tonal masking curves; the peakatt not only
       optionally specifies maximum dynamic depth, but also [always]
       limits the masking curves to a minimum depth  */
    for(i=0;i<P_BANDS;i++)
      for(j=0;j<P_LEVELS;j++){
	for(k=2;k<EHMER_OFFSET+2+vi->curvelimitp;k++)
	  if(p->tonecurves[i][j][k]> vi->peakatt->block[i][j])
	    p->tonecurves[i][j][k]=  vi->peakatt->block[i][j];
	  else
	    break;
      }
  }

  if(vi->peakattp) /* we limit depth only optionally */
    for(i=0;i<P_BANDS;i++)
      for(j=0;j<P_LEVELS;j++)
	if(p->tonecurves[i][j][EHMER_OFFSET+2]< vi->peakatt->block[i][j])
	  p->tonecurves[i][j][EHMER_OFFSET+2]=  vi->peakatt->block[i][j];

  /* but guarding is mandatory */
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      if(p->tonecurves[i][j][EHMER_OFFSET+2]< vi->tone_maxatt)
	  p->tonecurves[i][j][EHMER_OFFSET+2]=  vi->tone_maxatt;

  /* set up rolling noise median */
  for(i=0;i<n;i++){
    float halfoc=toOC((i+.5)*rate/(2.*n))*2.;
    int inthalfoc;
    float del;
    
    if(halfoc<0)halfoc=0;
    if(halfoc>=P_BANDS-1)halfoc=P_BANDS-1;
    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;
    p->noiseoffset[i]=
      p->vi->noiseoff[inthalfoc]*(1.-del) + 
      p->vi->noiseoff[inthalfoc+1]*del;
  }

  analysis_noisy=1;
  _analysis_output("noiseoff",0,p->noiseoffset,n,1,0);
  _analysis_output("noisethresh",0,p->noisethresh,n,1,0);

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);
  analysis_noisy=1;

}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)_ogg_free(p->ath);
    if(p->octave)_ogg_free(p->octave);
    if(p->bark)_ogg_free(p->bark);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
	for(j=0;j<P_LEVELS;j++){
	  _ogg_free(p->tonecurves[i][j]);
	}
	_ogg_free(p->tonecurves[i]);
      }
      _ogg_free(p->tonecurves);
    }
    _ogg_free(p->noiseoffset);
    _ogg_free(p->noisethresh);
    memset(p,0,sizeof(vorbis_look_psy));
  }
}

/* octave/(8*eighth_octave_lines) x scale and dB y scale */
static void seed_curve(float *seed,
		       const float **curves,
		       float amp,
		       int oc, int n,
		       int linesper,float dBoffset){
  int i,post1;
  int seedptr;
  const float *posts,*curve;

  int choice=(int)((amp+dBoffset)*.1f);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);
  posts=curves[choice];
  curve=posts+2;
  post1=(int)posts[1];
  seedptr=oc+(posts[0]-16)*linesper-(linesper>>1);

  for(i=posts[0];i<post1;i++){
    if(seedptr>0){
      float lin=amp+curve[i];
      if(seed[seedptr]<lin)seed[seedptr]=lin;
    }
    seedptr+=linesper;
    if(seedptr>=n)break;
  }
}

static void seed_loop(vorbis_look_psy *p,
		      const float ***curves,
		      const float *f, 
		      const float *flr,
		      float *seed,
		      float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  float dBoffset=vi->max_curve_dB-specmax;

  /* prime the working vector with peak values */

  for(i=0;i<n;i++){
    float max=f[i];
    long oc=p->octave[i];
    while(i+1<n && p->octave[i+1]==oc){
      i++;
      if(f[i]>max)max=f[i];
    }
    
    if(max+6.f>flr[i]){
      oc=oc>>p->shiftoc;
      if(oc>=P_BANDS)oc=P_BANDS-1;
      if(oc<0)oc=0;
      seed_curve(seed,
		 curves[oc],
		 max,
		 p->octave[i]-p->firstoc,
		 p->total_octave_lines,
		 p->eighth_octave_lines,
		 dBoffset);
    }
  }
}

static void seed_chase(float *seeds, int linesper, long n){
  long  *posstack=alloca(n*sizeof(long));
  float *ampstack=alloca(n*sizeof(float));
  long   stack=0;
  long   pos=0;
  long   i;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=seeds[i];
    }else{
      while(1){
	if(seeds[i]<ampstack[stack-1]){
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;
	}else{
	  if(i<posstack[stack-1]+linesper){
	    if(stack>1 && ampstack[stack-1]<=ampstack[stack-2] &&
	       i<posstack[stack-2]+linesper){
	      /* we completely overlap, making stack-1 irrelevant.  pop it */
	      stack--;
	      continue;
	    }
	  }
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;

	}
      }
    }
  }

  /* the stack now contains only the positions that are relevant. Scan
     'em straight through */

  for(i=0;i<stack;i++){
    long endpos;
    if(i<stack-1 && ampstack[i+1]>ampstack[i]){
      endpos=posstack[i+1];
    }else{
      endpos=posstack[i]+linesper+1; /* +1 is important, else bin 0 is
					discarded in short frames */
    }
    if(endpos>n)endpos=n;
    for(;pos<endpos;pos++)
      seeds[pos]=ampstack[i];
  }
  
  /* there.  Linear time.  I now remember this was on a problem set I
     had in Grad Skool... I didn't solve it at the time ;-) */

}

/* bleaugh, this is more complicated than it needs to be */
static void max_seeds(vorbis_look_psy *p,
		      vorbis_look_psy_global *g,
		      int channel,
		      float *seed,
		      float *flr){
  long   n=p->total_octave_lines;
  int    linesper=p->eighth_octave_lines;
  long   linpos=0;
  long   pos;

  seed_chase(seed,linesper,n); /* for masking */
 
  pos=p->octave[0]-p->firstoc-(linesper>>1);
  while(linpos+1<p->n){
    float minV=seed[pos];
    long end=((p->octave[linpos]+p->octave[linpos+1])>>1)-p->firstoc;
    while(pos+1<=end){
      pos++;
      if((seed[pos]>NEGINF && seed[pos]<minV) || minV==NEGINF)
	minV=seed[pos];
    }
    
    /* seed scale is log.  Floor is linear.  Map back to it */
    end=pos+p->firstoc;
    for(;linpos<p->n && p->octave[linpos]<=end;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
  {
    float minV=seed[p->total_octave_lines-1];
    for(;linpos<p->n;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
}

static void bark_noise_hybridmp(int n,const long *b,
				const float *f,
				float *noise,
				const float offset,
				const int fixed){
  long i,hi=b[0]>>16,lo=b[0]>>16,hif=-fixed/2,lof=-fixed/2;
  double xa=0,xb=0;
  double ya=0,yb=0;
  double x2a=0,x2b=0;
  double y2a=0,y2b=0;
  double xya=0,xyb=0; 
  double na=0,nb=0;

  for(i=0;i<n;i++){
    if(hi<n){
      /* find new lo/hi */
      int bi=b[i]&0xffffL;
      for(;hi<bi;hi++){
	int ii=(hi<0?-hi:hi);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	na  += nn;
	xa  += hi*nn;
	ya  += bin*nn;
	x2a += hi*hi*nn;
	y2a += bin*bin*nn;
	xya += hi*bin*nn;
      }
      bi=b[i]>>16;
      for(;lo<bi;lo++){
	int ii=(lo<0?-lo:lo);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	na  -= nn;
	xa  -= lo*nn;
	ya  -= bin*nn;
	x2a -= lo*lo*nn;
	y2a -= bin*bin*nn;
	xya -= lo*bin*nn;
      }
    }

    if(hif<n && fixed>0){
      int bi=i+fixed/2;
      if(bi>n)bi=n;

      for(;hif<bi;hif++){
	int ii=(hif<0?-hif:hif);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	nb  += nn;
	xb  += hif*nn;
	yb  += bin*nn;
	x2b += hif*hif*nn;
	y2b += bin*bin*nn;
	xyb += hif*bin*nn;
      }
      bi=i-(fixed+1)/2;
      for(;lof<bi;lof++){
	int ii=(lof<0?-lof:lof);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	nb  -= nn;
	xb  -= lof*nn;
	yb  -= bin*nn;
	x2b -= lof*lof*nn;
	y2b -= bin*bin*nn;
	xyb -= lof*bin*nn;
      }
    }

    {    
      double va=0.f;
      
      if(na>2){
        double denom=1./(na*x2a-xa*xa);
        double a=(ya*x2a-xya*xa)*denom;
        double b=(na*xya-xa*ya)*denom;
        va=a+b*i;
      }
      if(va<0.)va=0.;

      if(fixed>0){
        double vb=0.f;

        if(nb>2){
          double denomf=1./(nb*x2b-xb*xb);
          double af=(yb*x2b-xyb*xb)*denomf;
          double bf=(nb*xyb-xb*yb)*denomf;
          vb=af+bf*i;
        }
        if(vb<0.)vb=0.;
        if(va>vb && vb>0.)va=vb;

      }

      noise[i]=va-offset;
    }
  }
}

   
void _vp_remove_floor(vorbis_look_psy *p,
		      vorbis_look_psy_global *g,
		      float *logmdct, 
		      float *mdct,
		      float *codedflr,
		      float *residue,
		      float local_specmax){ 
  int i,n=p->n;
  
  for(i=0;i<n;i++)
    if(mdct[i]!=0.f)
      residue[i]=mdct[i]/codedflr[i];
    else
      residue[i]=0.f;
}
  

void _vp_compute_mask(vorbis_look_psy *p,
		       vorbis_look_psy_global *g,
		       int channel,
		       float *logfft, 
		       float *logmdct, 
		       float *logmask, 
		       float global_specmax,
		       float local_specmax,
		       int lastsize){
  int i,n=p->n;
  static int seq=0;

  float *seed=alloca(sizeof(float)*p->total_octave_lines);
  for(i=0;i<p->total_octave_lines;i++)seed[i]=NEGINF;

  /* noise masking */
  if(p->vi->noisemaskp){
    float *work=alloca(n*sizeof(float));

    bark_noise_hybridmp(n,p->bark,logmdct,logmask,
			140.,-1);

    for(i=0;i<n;i++)work[i]=logmdct[i]-logmask[i];

    _analysis_output("medianmdct",seq,work,n,1,0);
    bark_noise_hybridmp(n,p->bark,work,logmask,0.,
			p->vi->noisewindowfixed);

    for(i=0;i<n;i++)work[i]=logmdct[i]-work[i];

    /* work[i] holds the median line (.5), logmask holds the upper
       envelope line (1.) */

    _analysis_output("median",seq,work,n,1,0);

    _analysis_output("medianenvelope",seq,logmask,n,1,0);
    for(i=0;i<n;i++)logmask[i]+=work[i];
    _analysis_output("envelope",seq,logmask,n,1,0);
    for(i=0;i<n;i++)logmask[i]-=work[i];

    for(i=0;i<n;i++){
      int dB=logmask[i]+.5;
      if(dB>=NOISE_COMPAND_LEVELS)dB=NOISE_COMPAND_LEVELS-1;
      logmask[i]= work[i]+p->vi->noisecompand[dB]+p->noiseoffset[i];
    }

    _analysis_output("noise",seq,logmask,n,1,0);

  }else{
    for(i=0;i<n;i++)logmask[i]=NEGINF;
  }

  /* set the ATH (floating below localmax, not global max by a
     specified att) */
  if(p->vi->ath){
    float att=local_specmax+p->vi->ath_adjatt;
    if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;

    for(i=0;i<n;i++){
      float av=p->ath[i]+att;
      if(av>logmask[i])logmask[i]=av;
    }
  }

  /* tone/peak masking */
  seed_loop(p,(const float ***)p->tonecurves,logfft,logmask,seed,global_specmax);
  max_seeds(p,g,channel,seed,logmask);

  /* suppress any curve > p->vi->noisemaxsupp */
  if(p->vi->noisemaxsupp<0.f)
    for(i=0;i<n;i++)
      if(logmask[i]>p->vi->noisemaxsupp)
	logmask[i]=p->vi->noisemaxsupp;
  

  /* doing this here is clean, but we need to find a faster way to do
     it than to just tack it on */

  for(i=0;i<n;i++)if(logmdct[i]>=logmask[i])break;
  if(i==n)
    for(i=0;i<n;i++)logmask[i]=NEGINF;
  else
    for(i=0;i<n;i++)
      logfft[i]=max(logmdct[i],logfft[i]);

  seq++;

}

float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd){
  vorbis_info *vi=vd->vi;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=ci->psy_g_param;

  int n=ci->blocksizes[vd->W]/2;
  float secs=(float)n/vi->rate;

  amp+=secs*gi->ampmax_att_per_sec;
  if(amp<-9999)amp=-9999;
  return(amp);
}

static void couple_lossless(float A, float B, 
			    float granule,float igranule,
			    float *mag, float *ang){

  A=rint(A*igranule)*granule;
  B=rint(B*igranule)*granule;
  
  if(fabs(A)>fabs(B)){
    *mag=A; *ang=(A>0.f?A-B:B-A);
  }else{
    *mag=B; *ang=(B>0.f?A-B:B-A);
  }

  if(*ang>fabs(*mag)*1.9999f)*ang=-fabs(*mag)*2.f;
 
}

static void couple_8phase(float A, float B, float fA, float fB, 
			 float granule,float igranule,
			 float fmag, float *mag, float *ang){

  float origmag=FAST_HYPOT(A*fA,B*fB),corr;

  if(fmag!=0.f){
    float phase=rint((A-B)/fmag);
    
    if(fabs(A)>fabs(B)){
      *mag=A;phase=(A>0?phase:-phase);
    }else{
      *mag=B;phase=(B>0?phase:-phase);
    }
    
    switch((int)phase){
    case 0:
      corr=origmag/FAST_HYPOT(fmag*fA,fmag*fB);
      *mag=rint(*mag*corr*igranule)*granule; 
      *ang=0.f;
      break;
    case 1:
      corr=origmag/(fmag*fA);
      *mag=rint(A*corr*igranule)*granule; 
      *ang=fabs(*mag);
      break;
    case -1:
      corr=origmag/(fmag*fB);
      *mag=rint(B*corr*igranule)*granule; 
      *ang=-fabs(*mag);
      break;
    default:
      corr=origmag/FAST_HYPOT(fmag*fA,fmag*fB);
      *mag=rint(*mag*corr*igranule)*granule; 
      *ang=-2.f*fabs(*mag);
      break;
    }
  }else{
    *mag=0.f;
    *ang=0.f;
  }    
}

static void couple_6phase(float A, float B, float fA, float fB, 
			 float granule,float igranule,
			 float fmag, float *mag, float *ang){

  float origmag=FAST_HYPOT(A*fA,B*fB),corr;

  if(fmag!=0.f){
    float phase=rint((A-B)/fmag);
    
    if(fabs(A)>fabs(B)){
      *mag=A;phase=(A>0?phase:-phase);
    }else{
      *mag=B;phase=(B>0?phase:-phase);
    }
    
    switch((int)phase){
    case 0:
      corr=origmag/FAST_HYPOT(fmag*fA,fmag*fB);
      *mag=rint(*mag*corr*igranule)*granule; 
      *ang=0.f;
      break;
    case 1:case 2:
      corr=origmag/(fmag*fA);
      *mag=rint(A*corr*igranule)*granule; 
      *ang=fabs(*mag);
      break;
    case -1:case -2:
      corr=origmag/(fmag*fB);
      *mag=rint(B*corr*igranule)*granule; 
      *ang=-fabs(*mag);
      break;
    }
  }else{
    *mag=0.f;
    *ang=0.f;
  }    
}

static void couple_4phase(float A, float B, float fA, float fB, 
			 float granule,float igranule,
			 float fmag, float *mag, float *ang){

  float origmag=FAST_HYPOT(A*fA,B*fB),corr;

  if(fmag!=0.f){
    float phase=rint((A-B)*.5/fmag);
    
    if(fabs(A)>fabs(B)){
      *mag=A;phase=(A>0?phase:-phase);
    }else{
      *mag=B;phase=(B>0?phase:-phase);
    }
    
    corr=origmag/FAST_HYPOT(fmag*fA,fmag*fB);
    *mag=rint(*mag*corr*igranule)*granule; 
    switch((int)phase){
    case 0:
      *ang=0.f;
      break;
    default:
      *ang=-2.f*fabs(*mag);
      break;
    }
  }else{
    *mag=0.f;
    *ang=0.f;
  }    
}

static void couple_point(float A, float B, float fA, float fB, 
			 float granule,float igranule,
			 float fmag, float *mag, float *ang){

  float origmag=FAST_HYPOT(A*fA,B*fB),corr;

  if(fmag!=0.f){
    float phase=rint((A-B)*.5/fmag);
    
    if(fabs(A)>fabs(B)){
      *mag=A;phase=(A>0?phase:-phase);
    }else{
      *mag=B;phase=(B>0?phase:-phase);
    }
    
    //switch((int)phase){
      //case 0:
      corr=origmag/FAST_HYPOT(fmag*fA,fmag*fB);
      *mag=rint(*mag*corr*igranule)*granule; 
      *ang=0.f;
      //break;
      //default:
      //*mag=0.f;
      //*ang=0.f;
      //break;
      //}
  }else{
    *mag=0.f;
    *ang=0.f;
  }    
}


void _vp_quantize_couple(vorbis_look_psy *p,
			 vorbis_info_mapping0 *vi,
			 float **pcm,
			 float **sofar,
			 float **quantized,
			 int   *nonzero,
			 int   passno){

  int i,j,k,n=p->n;
  vorbis_info_psy *info=p->vi;

  /* perform any requested channel coupling */
  for(i=0;i<vi->coupling_steps;i++){
    float granulem=info->couple_pass[passno].granulem;
    float igranulem=info->couple_pass[passno].igranulem;
    
    /* make sure coupling a zero and a nonzero channel results in two
       nonzero channels. */
    if(nonzero[vi->coupling_mag[i]] ||
       nonzero[vi->coupling_ang[i]]){
      
      float *pcmM=pcm[vi->coupling_mag[i]];
      float *pcmA=pcm[vi->coupling_ang[i]];
      float *floorM=pcm[vi->coupling_mag[i]]+n;
      float *floorA=pcm[vi->coupling_ang[i]]+n;
      float *sofarM=sofar[vi->coupling_mag[i]];
      float *sofarA=sofar[vi->coupling_ang[i]];
      float *qM=quantized[vi->coupling_mag[i]];
      float *qA=quantized[vi->coupling_ang[i]];

      nonzero[vi->coupling_mag[i]]=1; 
      nonzero[vi->coupling_ang[i]]=1; 

      for(j=0,k=0;j<n;k++){
	vp_couple *part=info->couple_pass[passno].couple_pass+k;

	for(;j<part->limit && j<p->n;j++){
	  /* partition by partition; k is our by-location partition
	     class counter */
	  float ang,mag,fmag=max(fabs(pcmM[j]),fabs(pcmA[j]));

	  if(fmag<part->amppost_point){
	    couple_point(pcmM[j],pcmA[j],floorM[j],floorA[j],
			 granulem,igranulem,fmag,&mag,&ang);
	  }else{
	    if(fmag<part->amppost_6phase){
	      couple_6phase(pcmM[j],pcmA[j],floorM[j],floorA[j],
			   granulem,igranulem,fmag,&mag,&ang);
	    }else{ 
	      if(fmag<part->amppost_8phase){
		couple_8phase(pcmM[j],pcmA[j],floorM[j],floorA[j],
			      granulem,igranulem,fmag,&mag,&ang);
	      }else{
		couple_lossless(pcmM[j],pcmA[j],
				granulem,igranulem,&mag,&ang);
	      }
	    }
	  }
	  
	  qM[j]=mag-sofarM[j];
	  qA[j]=ang-sofarA[j];
	}
      }
    }
  }
}
