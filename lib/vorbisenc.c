/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: simple programmatic interface for encoder mode setup
 last mod: $Id: vorbisenc.c,v 1.43 2002/06/30 08:31:01 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

#include "codec_internal.h"

#include "os.h"
#include "misc.h"

/* careful with this; it's using static array sizing to make managing
   all the modes a little less annoying.  If we use a residue backend
   with > 12 partition types, or a different division of iteration,
   this needs to be updated. */
typedef struct {
  static_codebook *books[12][3];
} static_bookblock;

typedef struct {
  int res_type;
  int limit_type; /* 0 lowpass limited, 1 point stereo limited */
  vorbis_info_residue0 *res;
  static_codebook  *book_aux;
  static_codebook  *book_aux_managed;
  static_bookblock *books_base;
  static_bookblock *books_base_managed;
} vorbis_residue_template;

typedef struct {
  vorbis_info_mapping0    *map;
  vorbis_residue_template *res;
} vorbis_mapping_template;

typedef struct vp_adjblock{
  int block[P_BANDS];
} vp_adjblock;

typedef struct {
  int data[NOISE_COMPAND_LEVELS];
} compandblock;

/* high level configuration information for setting things up
   step-by-step with the detailed vorbis_encode_ctl interface.
   There's a fair amount of redundancy such that interactive setup
   does not directly deal with any vorbis_info or codec_setup_info
   initialization; it's all stored (until full init) in this highlevel
   setup, then flushed out to the real codec setup structs later. */

typedef struct {
  int att[P_NOISECURVES];
  float boost;
  float decay;
} att3;
typedef struct { int data[P_NOISECURVES]; } adj3; 

typedef struct {
  int   pre[PACKETBLOBS];
  int   post[PACKETBLOBS];
  float kHz[PACKETBLOBS];
  float lowpasskHz[PACKETBLOBS];
} adj_stereo;

typedef struct {
  int lo;
  int hi;
  int fixed;
} noiseguard;
typedef struct {
  int data[P_NOISECURVES][17];
} noise3;

typedef struct {
  int      mappings;
  double  *rate_mapping;
  double  *quality_mapping;
  int      coupling_restriction;
  long     samplerate_min_restriction;
  long     samplerate_max_restriction;


  int     *blocksize_short;
  int     *blocksize_long;

  att3    *psy_tone_masteratt;
  int     *psy_tone_0dB;
  int     *psy_tone_dBsuppress;

  vp_adjblock *psy_tone_adj_impulse;
  vp_adjblock *psy_tone_adj_long;
  vp_adjblock *psy_tone_adj_other;

  noiseguard  *psy_noiseguards;
  noise3      *psy_noise_bias_impulse;
  noise3      *psy_noise_bias_padding;
  noise3      *psy_noise_bias_trans;
  noise3      *psy_noise_bias_long;
  int         *psy_noise_dBsuppress;

  compandblock  *psy_noise_compand;
  double        *psy_noise_compand_short_mapping;
  double        *psy_noise_compand_long_mapping;

  int      *psy_noise_normal_start[2];
  int      *psy_noise_normal_partition[2];
  double   *psy_noise_normal_thresh;

  int      *psy_ath_float;
  int      *psy_ath_abs;

  double   *psy_lowpass;

  vorbis_info_psy_global *global_params;
  double     *global_mapping;
  adj_stereo *stereo_modes;

  static_codebook ***floor_books;
  vorbis_info_floor1 *floor_params;
  int *floor_short_mapping;
  int *floor_long_mapping;

  vorbis_mapping_template *maps;
} ve_setup_data_template;

#include "modes/setup_44.h"

static ve_setup_data_template *setup_list[]={
  &ve_setup_44_stereo,
  &ve_setup_44_stereo_low,
  0
};


/* a few static coder conventions */
static vorbis_info_mode _mode_template[2]={
  {0,0,0,0},
  {1,0,0,1}
};


static int vorbis_encode_toplevel_setup(vorbis_info *vi,int ch,long rate){
  if(vi && vi->codec_setup){

    vi->version=0;
    vi->channels=ch;
    vi->rate=rate;

    return(0);
  }
  return(OV_EINVAL);
}

static int vorbis_encode_floor_setup(vorbis_info *vi,double s,int block,
				     static_codebook    ***books, 
				     vorbis_info_floor1 *in, 
				     int *x){
  int i,k,is=rint(s);
  vorbis_info_floor1 *f=_ogg_calloc(1,sizeof(*f));
  codec_setup_info *ci=vi->codec_setup;

  memcpy(f,in+x[is],sizeof(*f));
  /* fill in the lowpass field, even if it's temporary */
  f->n=ci->blocksizes[block]>>1;

  /* books */
  {
    int partitions=f->partitions;
    int maxclass=-1;
    int maxbook=-1;
    for(i=0;i<partitions;i++)
      if(f->partitionclass[i]>maxclass)maxclass=f->partitionclass[i];
    for(i=0;i<=maxclass;i++){
      if(f->class_book[i]>maxbook)maxbook=f->class_book[i];
      f->class_book[i]+=ci->books;
      for(k=0;k<(1<<f->class_subs[i]);k++){
	if(f->class_subbook[i][k]>maxbook)maxbook=f->class_subbook[i][k];
	if(f->class_subbook[i][k]>=0)f->class_subbook[i][k]+=ci->books;
      }
    }

    for(i=0;i<=maxbook;i++)
      ci->book_param[ci->books++]=books[x[is]][i];
  }

  /* for now, we're only using floor 1 */
  ci->floor_type[ci->floors]=1;
  ci->floor_param[ci->floors]=f;
  ci->floors++;

  return(0);
}

static int vorbis_encode_global_psych_setup(vorbis_info *vi,double s,
					    vorbis_info_psy_global *in, 
					    double *x){
  int i,is=s;
  double ds=s-is;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *g=&ci->psy_g_param;
  
  memcpy(g,in+(int)x[is],sizeof(*g));
  
  ds=x[is]*(1.-ds)+x[is+1]*ds;
  is=(int)ds;
  ds-=is;
  if(ds==0 && is>0){
    is--;
    ds=1.;
  }
  
  /* interpolate the trigger threshholds */
  for(i=0;i<4;i++){
    g->preecho_thresh[i]=in[is].preecho_thresh[i]*(1.-ds)+in[is+1].preecho_thresh[i]*ds;
    g->postecho_thresh[i]=in[is].postecho_thresh[i]*(1.-ds)+in[is+1].postecho_thresh[i]*ds;
  }
  g->ampmax_att_per_sec=ci->hi.amplitude_track_dBpersec;
  return(0);
}

static int vorbis_encode_global_stereo(vorbis_info *vi,
				       highlevel_encode_setup *hi,
				       adj_stereo *p){
  float s=hi->stereo_point_setting;
  int i,is=s;
  double ds=s-is;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *g=&ci->psy_g_param;

  memcpy(g->coupling_prepointamp,p[is].pre,sizeof(*p[is].pre)*PACKETBLOBS);
  memcpy(g->coupling_postpointamp,p[is].post,sizeof(*p[is].post)*PACKETBLOBS);

  if(hi->managed){
    /* interpolate the kHz threshholds */
    for(i=0;i<PACKETBLOBS;i++){
      float kHz=p[is].kHz[i]*(1.-ds)+p[is+1].kHz[i]*ds;
      g->coupling_pointlimit[0][i]=kHz*1000./vi->rate*ci->blocksizes[0];
      g->coupling_pointlimit[1][i]=kHz*1000./vi->rate*ci->blocksizes[1];
      g->coupling_pkHz[i]=kHz;

      kHz=p[is].lowpasskHz[i]*(1.-ds)+p[is+1].lowpasskHz[i]*ds;
      g->sliding_lowpass[0][i]=kHz*1000./vi->rate*ci->blocksizes[0];
      g->sliding_lowpass[1][i]=kHz*1000./vi->rate*ci->blocksizes[1];

    }
  }else{
    float kHz=p[is].kHz[PACKETBLOBS/2]*(1.-ds)+p[is+1].kHz[PACKETBLOBS/2]*ds;
    for(i=0;i<PACKETBLOBS;i++){
      g->coupling_pointlimit[0][i]=kHz*1000./vi->rate*ci->blocksizes[0];
      g->coupling_pointlimit[1][i]=kHz*1000./vi->rate*ci->blocksizes[1];
      g->coupling_pkHz[i]=kHz;
    }

    kHz=p[is].lowpasskHz[PACKETBLOBS/2]*(1.-ds)+p[is+1].lowpasskHz[PACKETBLOBS/2]*ds;
    for(i=0;i<PACKETBLOBS;i++){
      g->sliding_lowpass[0][i]=kHz*1000./vi->rate*ci->blocksizes[0];
      g->sliding_lowpass[1][i]=kHz*1000./vi->rate*ci->blocksizes[1];
    }
  }

  return(0);
}

static int vorbis_encode_psyset_setup(vorbis_info *vi,double s,
				      int *nn_start,
				      int *nn_partition,
				      double *nn_thresh,
				      int block){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];
  highlevel_encode_setup *hi=&ci->hi;
  int is=s;
  
  if(block>=ci->psys)
    ci->psys=block+1;
  if(!p){
    p=_ogg_calloc(1,sizeof(*p));
    ci->psy_param[block]=p;
  }
  
  memcpy(p,&_psy_info_template,sizeof(*p));
  p->blockflag=block>>1;

  if(hi->noise_normalize_p){
    p->normal_channel_p=1;
    p->normal_point_p=1;
    p->normal_start=nn_start[is];
    p->normal_partition=nn_partition[is];
    p->normal_thresh=nn_thresh[is];
  }
    
  return 0;
}

static int vorbis_encode_tonemask_setup(vorbis_info *vi,double s,int block,
					att3 *att,
					int  *max,
					vp_adjblock *in){
  int i,is=s;
  double ds=s-is;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  /* 0 and 2 are only used by bitmanagement, but there's no harm to always
     filling the values in here */
  p->tone_masteratt[0]=att[is].att[0]*(1.-ds)+att[is+1].att[0]*ds;
  p->tone_masteratt[1]=att[is].att[1]*(1.-ds)+att[is+1].att[1]*ds;
  p->tone_masteratt[2]=att[is].att[2]*(1.-ds)+att[is+1].att[2]*ds;
  p->tone_centerboost=att[is].boost*(1.-ds)+att[is+1].boost*ds;
  p->tone_decay=att[is].decay*(1.-ds)+att[is+1].decay*ds;

  p->max_curve_dB=max[is]*(1.-ds)+max[is+1]*ds;

  for(i=0;i<P_BANDS;i++)
    p->toneatt[i]=in[is].block[i]*(1.-ds)+in[is+1].block[i]*ds;
  return(0);
}


static int vorbis_encode_compand_setup(vorbis_info *vi,double s,int block,
				       compandblock *in, double *x){
  int i,is=s;
  double ds=s-is;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  ds=x[is]*(1.-ds)+x[is+1]*ds;
  is=(int)ds;
  ds-=is;
  if(ds==0 && is>0){
    is--;
    ds=1.;
  }

  /* interpolate the compander settings */
  for(i=0;i<NOISE_COMPAND_LEVELS;i++)
    p->noisecompand[i]=in[is].data[i]*(1.-ds)+in[is+1].data[i]*ds;
  return(0);
}

static int vorbis_encode_peak_setup(vorbis_info *vi,double s,int block,
				    int *suppress){
  int is=s;
  double ds=s-is;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  p->tone_abs_limit=suppress[is]*(1.-ds)+suppress[is+1]*ds;

  return(0);
}

static int vorbis_encode_noisebias_setup(vorbis_info *vi,double s,int block,
					 int *suppress,
					 noise3 *in,
					 noiseguard *guard){
  int i,is=s,j;
  double ds=s-is;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  p->noisemaxsupp=suppress[is]*(1.-ds)+suppress[is+1]*ds;
  p->noisewindowlomin=guard[block].lo;
  p->noisewindowhimin=guard[block].hi;
  p->noisewindowfixed=guard[block].fixed;

  for(j=0;j<P_NOISECURVES;j++)
    for(i=0;i<P_BANDS;i++)
      p->noiseoff[j][i]=in[is].data[j][i]*(1.-ds)+in[is+1].data[j][i]*ds;

  return(0);
}

static int vorbis_encode_ath_setup(vorbis_info *vi,int block){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  p->ath_adjatt=ci->hi.ath_floating_dB;
  p->ath_maxatt=ci->hi.ath_absolute_dB;
  return(0);
}


static int book_dup_or_new(codec_setup_info *ci,static_codebook *book){
  int i;
  for(i=0;i<ci->books;i++)
    if(ci->book_param[i]==book)return(i);
  
  return(ci->books++);
}

static void vorbis_encode_blocksize_setup(vorbis_info *vi,double s,
					 int *shortb,int *longb){

  codec_setup_info *ci=vi->codec_setup;
  int is=s;
  
  int blockshort=shortb[is];
  int blocklong=longb[is];
  ci->blocksizes[0]=blockshort;
  ci->blocksizes[1]=blocklong;

}

static void vorbis_encode_residue_setup(vorbis_info *vi,
				       int number, int block,
				       vorbis_residue_template *res){

  codec_setup_info *ci=vi->codec_setup;
  int i,n;
  
  vorbis_info_residue0 *r=ci->residue_param[number]=
    _ogg_malloc(sizeof(*r));
  
  memcpy(r,res->res,sizeof(*r));
  if(ci->residues<=number)ci->residues=number+1;

  switch(ci->blocksizes[block]){
  case 64:case 128:case 256:case 512:
    r->grouping=16;
    break;
  default:
    r->grouping=32;
    break;
  }
  ci->residue_type[number]=res->res_type;

  /* to be adjusted by lowpass/pointlimit later */
  n=r->end=ci->blocksizes[block]>>1; 
  if(res->res_type==2)
    n=r->end*=vi->channels;
  
  /* fill in all the books */
  {
    int booklist=0,k;
    
    if(ci->hi.managed){
      for(i=0;i<r->partitions;i++)
	for(k=0;k<3;k++)
	  if(res->books_base_managed->books[i][k])
	    r->secondstages[i]|=(1<<k);

      r->groupbook=book_dup_or_new(ci,res->book_aux_managed);
      ci->book_param[r->groupbook]=res->book_aux_managed;      
    
      for(i=0;i<r->partitions;i++){
	for(k=0;k<3;k++){
	  if(res->books_base_managed->books[i][k]){
	    int bookid=book_dup_or_new(ci,res->books_base_managed->books[i][k]);
	    r->booklist[booklist++]=bookid;
	    ci->book_param[bookid]=res->books_base_managed->books[i][k];
	  }
	}
      }

    }else{

      for(i=0;i<r->partitions;i++)
	for(k=0;k<3;k++)
	  if(res->books_base->books[i][k])
	    r->secondstages[i]|=(1<<k);
  
      r->groupbook=book_dup_or_new(ci,res->book_aux);
      ci->book_param[r->groupbook]=res->book_aux;
      
      for(i=0;i<r->partitions;i++){
	for(k=0;k<3;k++){
	  if(res->books_base->books[i][k]){
	    int bookid=book_dup_or_new(ci,res->books_base->books[i][k]);
	    r->booklist[booklist++]=bookid;
	    ci->book_param[bookid]=res->books_base->books[i][k];
	  }
	}
      }
    }
  }
  
  /* lowpass setup/pointlimit */
  {
    double freq=ci->hi.lowpass_kHz*1000.;
    vorbis_info_floor1 *f=ci->floor_param[block]; /* by convention */
    double nyq=vi->rate/2.;
    long blocksize=ci->blocksizes[block]>>1;

    /* lowpass needs to be set in the floor and the residue. */    
    if(freq>nyq)freq=nyq;
    /* in the floor, the granularity can be very fine; it doesn't alter
       the encoding structure, only the samples used to fit the floor
       approximation */
    f->n=freq/nyq*blocksize; 

    /* this res may by limited by the maximum pointlimit of the mode,
       not the lowpass. the floor is always lowpass limited. */
    if(res->limit_type){
      if(ci->hi.managed)
	freq=ci->psy_g_param.coupling_pkHz[PACKETBLOBS-1]*1000.;
      else
	freq=ci->psy_g_param.coupling_pkHz[PACKETBLOBS/2]*1000.;
      if(freq>nyq)freq=nyq;
    }
    
    /* in the residue, we're constrained, physically, by partition
       boundaries.  We still lowpass 'wherever', but we have to round up
       here to next boundary, or the vorbis spec will round it *down* to
       previous boundary in encode/decode */
    if(ci->residue_type[block]==2)
      r->end=(int)((freq/nyq*blocksize*2)/r->grouping+.9)* /* round up only if we're well past */
	r->grouping;
    else
      r->end=(int)((freq/nyq*blocksize)/r->grouping+.9)* /* round up only if we're well past */
	r->grouping;
  }
}      

/* we assume two maps in this encoder */
static void vorbis_encode_map_n_res_setup(vorbis_info *vi,double s,
					  vorbis_mapping_template *maps){

  codec_setup_info *ci=vi->codec_setup;
  int i,j,is=s;
  vorbis_info_mapping0 *map=maps[is].map;
  vorbis_info_mode *mode=_mode_template;
  vorbis_residue_template *res=maps[is].res;

  for(i=0;i<2;i++){

    ci->map_param[i]=_ogg_calloc(1,sizeof(*map));
    ci->mode_param[i]=_ogg_calloc(1,sizeof(*mode));
  
    memcpy(ci->mode_param[i],mode+i,sizeof(*_mode_template));
    if(i>=ci->modes)ci->modes=i+1;

    ci->map_type[i]=0;
    memcpy(ci->map_param[i],map+i,sizeof(*map));
    if(i>=ci->maps)ci->maps=i+1;
    
    for(j=0;j<map[i].submaps;j++)
      vorbis_encode_residue_setup(vi,map[i].residuesubmap[j],i
				  ,res+map[i].residuesubmap[j]);
  }
}

static double setting_to_approx_bitrate(vorbis_info *vi){
  codec_setup_info *ci=vi->codec_setup;
  highlevel_encode_setup *hi=&ci->hi;
  ve_setup_data_template *setup=(ve_setup_data_template *)hi->setup;
  int is=hi->base_setting;
  double ds=hi->base_setting-is;
  int ch=vi->channels;
  double *r=setup->rate_mapping;

  if(r==NULL)
    return(-1);
  
  return((r[is]*(1.-ds)+r[is+1]*ds)*ch);  
}

static void get_setup_template(vorbis_info *vi,
			       long ch,long srate,
			       double req,int q_or_bitrate){
  int i=0,j;
  codec_setup_info *ci=vi->codec_setup;
  highlevel_encode_setup *hi=&ci->hi;
  if(q_or_bitrate)req/=ch;

  while(setup_list[i]){
    if(setup_list[i]->coupling_restriction==-1 ||
       setup_list[i]->coupling_restriction==ch){
      if(srate>=setup_list[i]->samplerate_min_restriction &&
	 srate<=setup_list[i]->samplerate_max_restriction){
	int mappings=setup_list[i]->mappings;
	double *map=(q_or_bitrate?
		     setup_list[i]->rate_mapping:
		     setup_list[i]->quality_mapping);

	/* the template matches.  Does the requested quality mode
	   fall within this template's modes? */
	if(req<map[0]){++i;continue;}
	if(req>map[setup_list[i]->mappings]){++i;continue;}
	for(j=0;j<mappings;j++)
	  if(req>=map[j] && req<map[j+1])break;
	/* an all-points match */
	hi->setup=setup_list[i];
	if(j==mappings)
	  hi->base_setting=j-.001;
	else{
	  float low=map[j];
	  float high=map[j+1];
	  float del=(req-low)/(high-low);
	  hi->base_setting=j+del;
	}
	return;
      }
    }
    i++;
  }
  
  hi->setup=NULL;
}

/* encoders will need to use vorbis_info_init beforehand and call
   vorbis_info clear when all done */

/* two interfaces; this, more detailed one, and later a convenience
   layer on top */

/* the final setup call */
int vorbis_encode_setup_init(vorbis_info *vi){
  int ret=0,i0=0;
  codec_setup_info *ci=vi->codec_setup;
  ve_setup_data_template *setup=NULL;
  highlevel_encode_setup *hi=&ci->hi;

  if(ci==NULL)return(OV_EINVAL);
  if(!hi->impulse_block_p)i0=1;

  /* too low/high an ATH floater is nonsensical, but doesn't break anything */
  if(hi->ath_floating_dB>-80)hi->ath_floating_dB=-80;
  if(hi->ath_floating_dB<-200)hi->ath_floating_dB=-200;

  /* again, bound this to avoid the app shooting itself int he foot
     too badly */
  if(hi->amplitude_track_dBpersec>0.)hi->amplitude_track_dBpersec=0.;
  if(hi->amplitude_track_dBpersec<-99999.)hi->amplitude_track_dBpersec=-99999.;
  
  /* get the appropriate setup template; matches the fetch in previous
     stages */
  setup=(ve_setup_data_template *)hi->setup;
  if(setup==NULL)return(OV_EINVAL);

  hi->set_in_stone=1;
  /* choose block sizes from configured sizes as well as paying
     attention to long_block_p and short_block_p.  If the configured
     short and long blocks are the same length, we set long_block_p
     and unset short_block_p */
  vorbis_encode_blocksize_setup(vi,hi->base_setting,
				setup->blocksize_short,
				setup->blocksize_long);
  
  /* floor setup; choose proper floor params.  Allocated on the floor
     stack in order; if we alloc only long floor, it's 0 */
  ret|=vorbis_encode_floor_setup(vi,hi->short_setting,0,
				 setup->floor_books,
				 setup->floor_params,
				 setup->floor_short_mapping);
  ret|=vorbis_encode_floor_setup(vi,hi->long_setting,1,
				 setup->floor_books,
				 setup->floor_params,
				 setup->floor_long_mapping);
  
  /* setup of [mostly] short block detection and stereo*/
  ret|=vorbis_encode_global_psych_setup(vi,hi->trigger_setting,
					setup->global_params,
					setup->global_mapping);
  ret|=vorbis_encode_global_stereo(vi,hi,setup->stereo_modes);

  /* basic psych setup and noise normalization */
  ret|=vorbis_encode_psyset_setup(vi,hi->short_setting,
				  setup->psy_noise_normal_start[0],
				  setup->psy_noise_normal_partition[0],  
				  setup->psy_noise_normal_thresh,  
				  0);
  ret|=vorbis_encode_psyset_setup(vi,hi->short_setting,
				  setup->psy_noise_normal_start[0],
				  setup->psy_noise_normal_partition[0],  
				  setup->psy_noise_normal_thresh,  
				  1);
  ret|=vorbis_encode_psyset_setup(vi,hi->long_setting,
				  setup->psy_noise_normal_start[1],
				  setup->psy_noise_normal_partition[1],  
				  setup->psy_noise_normal_thresh,  
				  2);
  ret|=vorbis_encode_psyset_setup(vi,hi->long_setting,
				  setup->psy_noise_normal_start[1],
				  setup->psy_noise_normal_partition[1],  
				  setup->psy_noise_normal_thresh,  
				  3);

  /* tone masking setup */
  ret|=vorbis_encode_tonemask_setup(vi,hi->block[i0].tone_mask_setting,0,
				    setup->psy_tone_masteratt,
				    setup->psy_tone_0dB,
				    setup->psy_tone_adj_impulse);
  ret|=vorbis_encode_tonemask_setup(vi,hi->block[1].tone_mask_setting,1,
				    setup->psy_tone_masteratt,
				    setup->psy_tone_0dB,
				    setup->psy_tone_adj_other);
  ret|=vorbis_encode_tonemask_setup(vi,hi->block[2].tone_mask_setting,2,
				    setup->psy_tone_masteratt,
				    setup->psy_tone_0dB,
				    setup->psy_tone_adj_other);
  ret|=vorbis_encode_tonemask_setup(vi,hi->block[3].tone_mask_setting,3,
				    setup->psy_tone_masteratt,
				    setup->psy_tone_0dB,
				    setup->psy_tone_adj_long);

  /* noise companding setup */
  ret|=vorbis_encode_compand_setup(vi,hi->block[i0].noise_compand_setting,0,
				   setup->psy_noise_compand,
				   setup->psy_noise_compand_short_mapping);
  ret|=vorbis_encode_compand_setup(vi,hi->block[1].noise_compand_setting,1,
				   setup->psy_noise_compand,
				   setup->psy_noise_compand_short_mapping);
  ret|=vorbis_encode_compand_setup(vi,hi->block[2].noise_compand_setting,2,
				   setup->psy_noise_compand,
				   setup->psy_noise_compand_long_mapping);
  ret|=vorbis_encode_compand_setup(vi,hi->block[3].noise_compand_setting,3,
				   setup->psy_noise_compand,
				   setup->psy_noise_compand_long_mapping);

  /* peak guarding setup  */
  ret|=vorbis_encode_peak_setup(vi,hi->block[i0].tone_peaklimit_setting,0,
				setup->psy_tone_dBsuppress);
  ret|=vorbis_encode_peak_setup(vi,hi->block[1].tone_peaklimit_setting,1,
				setup->psy_tone_dBsuppress);
  ret|=vorbis_encode_peak_setup(vi,hi->block[2].tone_peaklimit_setting,2,
				setup->psy_tone_dBsuppress);
  ret|=vorbis_encode_peak_setup(vi,hi->block[3].tone_peaklimit_setting,3,
				setup->psy_tone_dBsuppress);

  /* noise bias setup */
  ret|=vorbis_encode_noisebias_setup(vi,hi->block[i0].noise_bias_setting,0,
				     setup->psy_noise_dBsuppress,
				     setup->psy_noise_bias_impulse,
				     setup->psy_noiseguards);
  ret|=vorbis_encode_noisebias_setup(vi,hi->block[1].noise_bias_setting,1,
				     setup->psy_noise_dBsuppress,
				     setup->psy_noise_bias_padding,
				     setup->psy_noiseguards);
  ret|=vorbis_encode_noisebias_setup(vi,hi->block[2].noise_bias_setting,2,
				     setup->psy_noise_dBsuppress,
				     setup->psy_noise_bias_trans,
				     setup->psy_noiseguards);
  ret|=vorbis_encode_noisebias_setup(vi,hi->block[3].noise_bias_setting,3,
				     setup->psy_noise_dBsuppress,
				     setup->psy_noise_bias_long,
				     setup->psy_noiseguards);

  ret|=vorbis_encode_ath_setup(vi,0);
  ret|=vorbis_encode_ath_setup(vi,1);
  ret|=vorbis_encode_ath_setup(vi,2);
  ret|=vorbis_encode_ath_setup(vi,3);

  if(ret){
    vorbis_info_clear(vi);
    return ret; 
  }

  vorbis_encode_map_n_res_setup(vi,hi->base_setting,setup->maps);

  /* set bitrate readonlies and management */
  vi->bitrate_nominal=setting_to_approx_bitrate(vi);
  vi->bitrate_lower=hi->bitrate_min;
  vi->bitrate_upper=hi->bitrate_max;
  vi->bitrate_window=hi->bitrate_limit_window;

  if(hi->managed){
    ci->bi.queue_avg_time=hi->bitrate_av_window;
    ci->bi.queue_avg_center=hi->bitrate_av_window_center;
    ci->bi.queue_minmax_time=hi->bitrate_limit_window;
    ci->bi.queue_hardmin=hi->bitrate_min;
    ci->bi.queue_hardmax=hi->bitrate_max;
    ci->bi.queue_avgmin=hi->bitrate_av_lo;
    ci->bi.queue_avgmax=hi->bitrate_av_hi;
    ci->bi.avgfloat_downslew_max=-999999.f;
    ci->bi.avgfloat_upslew_max=999999.f;
  }

  return(ret);
  
}

static int vorbis_encode_setup_setting(vorbis_info *vi,
				       long  channels,
				       long  rate){
  int ret=0,i,is;
  codec_setup_info *ci=vi->codec_setup;
  highlevel_encode_setup *hi=&ci->hi;
  ve_setup_data_template *setup=hi->setup;
  double ds;

  ret=vorbis_encode_toplevel_setup(vi,channels,rate);
  if(ret)return(ret);

  is=hi->base_setting;
  ds=hi->base_setting-is;

  hi->short_setting=hi->base_setting;
  hi->long_setting=hi->base_setting;

  hi->managed=0;

  hi->impulse_block_p=1;
  hi->noise_normalize_p=1;

  hi->stereo_point_setting=hi->base_setting;
  hi->lowpass_kHz=
    setup->psy_lowpass[is]*(1.-ds)+setup->psy_lowpass[is+1]*ds;  
  
  hi->ath_floating_dB=setup->psy_ath_float[is]*(1.-ds)+
    setup->psy_ath_float[is+1]*ds;
  hi->ath_absolute_dB=setup->psy_ath_abs[is]*(1.-ds)+
    setup->psy_ath_abs[is+1]*ds;

  hi->amplitude_track_dBpersec=-6.;
  hi->trigger_setting=hi->base_setting;

  for(i=0;i<4;i++){
    hi->block[i].tone_mask_setting=hi->base_setting;
    hi->block[i].tone_peaklimit_setting=hi->base_setting;
    hi->block[i].noise_bias_setting=hi->base_setting;
    hi->block[i].noise_compand_setting=hi->base_setting;
  }

  return(ret);
}

int vorbis_encode_setup_vbr(vorbis_info *vi,
			    long  channels,
			    long  rate,			    
			    float quality){
  codec_setup_info *ci=vi->codec_setup;
  highlevel_encode_setup *hi=&ci->hi;

  quality+=.00001;

  get_setup_template(vi,channels,rate,quality,0);
  if(!hi->setup)return OV_EIMPL;
  
  return vorbis_encode_setup_setting(vi,channels,rate);
}

int vorbis_encode_init_vbr(vorbis_info *vi,
			   long channels,
			   long rate,
			   
			   float base_quality /* 0. to 1. */
			   ){
  int ret=0;

  ret=vorbis_encode_setup_vbr(vi,channels,rate,base_quality);
  
  if(ret){
    vorbis_info_clear(vi);
    return ret; 
  }
  ret=vorbis_encode_setup_init(vi);
  if(ret)
    vorbis_info_clear(vi);
  return(ret);
}

int vorbis_encode_setup_managed(vorbis_info *vi,
				long channels,
				long rate,
				
				long max_bitrate,
				long nominal_bitrate,
				long min_bitrate){

  codec_setup_info *ci=vi->codec_setup;
  highlevel_encode_setup *hi=&ci->hi;
  double tnominal=nominal_bitrate;
  int ret=0;

  if(nominal_bitrate<=0.){
    if(max_bitrate>0.){
      nominal_bitrate=max_bitrate*.875;
    }else{
      if(min_bitrate>0.){
	nominal_bitrate=min_bitrate;
      }else{
	return(OV_EINVAL);
      }
    }
  }

  get_setup_template(vi,channels,rate,nominal_bitrate,1);
  if(!hi->setup)return OV_EIMPL;
  
  ret=vorbis_encode_setup_setting(vi,channels,rate);
  if(ret){
    vorbis_info_clear(vi);
    return ret; 
  }

  /* initialize management with sane defaults */
      /* initialize management with sane defaults */
  hi->managed=1;
  hi->bitrate_av_window=4.;
  hi->bitrate_av_window_center=.5;
  hi->bitrate_limit_window=2.;
  hi->bitrate_min=min_bitrate;
  hi->bitrate_max=max_bitrate;
  hi->bitrate_av_lo=tnominal;
  hi->bitrate_av_hi=tnominal;

  return(ret);

}

int vorbis_encode_init(vorbis_info *vi,
		       long channels,
		       long rate,

		       long max_bitrate,
		       long nominal_bitrate,
		       long min_bitrate){

  int ret=vorbis_encode_setup_managed(vi,channels,rate,
				      max_bitrate,
				      nominal_bitrate,
				      min_bitrate);
  if(ret){
    vorbis_info_clear(vi);
    return(ret);
  }

  ret=vorbis_encode_setup_init(vi);
  if(ret)
    vorbis_info_clear(vi);
  return(ret);
}

int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg){
  if(vi){
    codec_setup_info *ci=vi->codec_setup;
    highlevel_encode_setup *hi=&ci->hi;
    int setp=(number&0xf); /* a read request has a low nibble of 0 */

    if(setp && hi->set_in_stone)return(OV_EINVAL);

    switch(number){
    case OV_ECTL_RATEMANAGE_GET:
      {
	
	struct ovectl_ratemanage_arg *ai=
	  (struct ovectl_ratemanage_arg *)arg;
	
	ai->management_active=hi->managed;
	ai->bitrate_av_window=hi->bitrate_av_window;
	ai->bitrate_av_window_center=hi->bitrate_av_window_center;
	ai->bitrate_hard_window=hi->bitrate_limit_window;
	ai->bitrate_hard_min=hi->bitrate_min;
	ai->bitrate_hard_max=hi->bitrate_max;
	ai->bitrate_av_lo=hi->bitrate_av_lo;
	ai->bitrate_av_hi=hi->bitrate_av_hi;
	
      }
      return(0);
    
    case OV_ECTL_RATEMANAGE_SET:
      {
	struct ovectl_ratemanage_arg *ai=
	  (struct ovectl_ratemanage_arg *)arg;
	if(ai==NULL){
	  hi->managed=0;
	}else{
	  hi->managed=ai->management_active;
	  vorbis_encode_ctl(vi,OV_ECTL_RATEMANAGE_AVG,arg);
	  vorbis_encode_ctl(vi,OV_ECTL_RATEMANAGE_HARD,arg);
	}
      }
      return 0;

    case OV_ECTL_RATEMANAGE_AVG:
      {
	struct ovectl_ratemanage_arg *ai=
	  (struct ovectl_ratemanage_arg *)arg;
	if(ai==NULL){
	  hi->bitrate_av_lo=0;
	  hi->bitrate_av_hi=0;
	  hi->bitrate_av_window=0;
	}else{
	  hi->bitrate_av_window=ai->bitrate_av_window;
	  hi->bitrate_av_window_center=ai->bitrate_av_window_center;
	  hi->bitrate_av_lo=ai->bitrate_av_lo;
	  hi->bitrate_av_hi=ai->bitrate_av_hi;
	}

	if(hi->bitrate_av_window<.25)hi->bitrate_av_window=.25;
	if(hi->bitrate_av_window>10.)hi->bitrate_av_window=10.;
	if(hi->bitrate_av_window_center<0.)hi->bitrate_av_window=0.;
	if(hi->bitrate_av_window_center>1.)hi->bitrate_av_window=1.;
	
	if( ( (hi->bitrate_av_lo<=0 && hi->bitrate_av_hi<=0)||
	      (hi->bitrate_av_window<=0) ) &&
	    ( (hi->bitrate_min<=0 && hi->bitrate_max<=0)||
	      (hi->bitrate_limit_window<=0) ))
	  hi->managed=0;
      }
      return(0);
    case OV_ECTL_RATEMANAGE_HARD:
      {
	struct ovectl_ratemanage_arg *ai=
	  (struct ovectl_ratemanage_arg *)arg;
	if(ai==NULL){
	  hi->bitrate_min=0;
	  hi->bitrate_max=0;
	  hi->bitrate_limit_window=0;
	}else{
	  hi->bitrate_limit_window=ai->bitrate_hard_window;
	  hi->bitrate_min=ai->bitrate_hard_min;
	  hi->bitrate_max=ai->bitrate_hard_max;
	}
	if(hi->bitrate_limit_window<0.)hi->bitrate_limit_window=0.;
	if(hi->bitrate_limit_window>10.)hi->bitrate_limit_window=10.;
	
	if( ( (hi->bitrate_av_lo<=0 && hi->bitrate_av_hi<=0)||
	      (hi->bitrate_av_window<=0) ) &&
	    ( (hi->bitrate_min<=0 && hi->bitrate_max<=0)||
	      (hi->bitrate_limit_window<=0) ))
	  hi->managed=0;
      }
      return(0);
    }

    return(OV_EIMPL);
  }
  return(OV_EINVAL);
}
