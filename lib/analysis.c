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

 function: single-block PCM analysis mode dispatch
 last mod: $Id: analysis.c,v 1.52 2002/06/28 22:19:34 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "scales.h"
#include "os.h"
#include "misc.h"

int analysis_noisy=1;

/* decides between modes, dispatches to the appropriate mapping. */
int vorbis_analysis(vorbis_block *vb, ogg_packet *op){
  int                   ret;

  vb->glue_bits=0;
  vb->time_bits=0;
  vb->floor_bits=0;
  vb->res_bits=0;

  /* first things first.  Make sure encode is ready */
  oggpack_reset(&vb->opb);
  
  /* we only have one mapping type (0), and we let the mapping code
     itself figure out what soft mode to use.  This allows easier
     bitrate management */

  if((ret=_mapping_P[0]->forward(vb)))
    return(ret);

  if(op){
    if(vorbis_bitrate_managed(vb))
      /* The app is using a bitmanaged mode... but not using the
         bitrate management interface. */
      return(OV_EINVAL);
    
    op->packet=oggpack_get_buffer(&vb->opb);
    op->bytes=oggpack_bytes(&vb->opb);
    op->b_o_s=0;
    op->e_o_s=vb->eofflag;
    op->granulepos=vb->granulepos;
    op->packetno=vb->sequence; /* for sake of completeness */
  }
  return(0);
}

/* there was no great place to put this.... */
void _analysis_output_always(char *base,int i,float *v,int n,int bark,int dB,ogg_int64_t off){
  int j;
  FILE *of;
  char buffer[80];

  /*  if(i==5870){*/
    sprintf(buffer,"%s_%d.m",base,i);
    of=fopen(buffer,"w");
    
    if(!of)perror("failed to open data dump file");
    
    for(j=0;j<n;j++){
      if(dB && v[j]==0)
	fprintf(of,"\n\n");
      else{
	if(v[j]>-900.){
	  if(bark)
	    fprintf(of,"%f ",toBARK(22050.f*j/n));
	  else
	    if(off!=0)
	      fprintf(of,"%f ",(double)(j+off)/44100.);
	    else
	    fprintf(of,"%f ",(double)j);
	  
	  if(dB){
	    fprintf(of,"%f\n",todB(v+j));
	  }else{
	    fprintf(of,"%f\n",v[j]);
	  }
	}
      }
    }
    fclose(of);
    /*  } */
}

void _analysis_output(char *base,int i,float *v,int n,int bark,int dB,
		      ogg_int64_t off){
  if(analysis_noisy)_analysis_output_always(base,i,v,n,bark,dB,off);
}













