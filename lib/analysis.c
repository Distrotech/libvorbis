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

 function: single-block PCM analysis
 last mod: $Id: analysis.c,v 1.20 2000/01/05 03:10:53 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"

#include "os.h"
#include "lpc.h"
#include "lsp.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"
#include "bitwise.h"
#include "spectrum.h"

/* this code is still seriously abbreviated.  I'm filling in pieces as
   we go... --Monty 19991004 */

int vorbis_analysis(vorbis_block *vb,ogg_packet *op){
  int i;
  double           *window=vb->vd->window[vb->W][vb->lW][vb->nW];
  psy_lookup       *vp=&vb->vd->vp[vb->W];
  lpc_lookup       *vl=&vb->vd->vl[vb->W];
  vorbis_dsp_state *vd=vb->vd;
  vorbis_info      *vi=vd->vi;
  oggpack_buffer   *opb=&vb->opb;

  int              n=vb->pcmend;
  int              spectral_order=vi->floororder[vb->W];

  vb->gluebits=0;
  vb->time_envelope_bits=0;
  vb->spectral_envelope_bits=0;
  vb->spectral_residue_bits=0;

  /*lpc_lookup       *vbal=&vb->vd->vbal[vb->W];
    double balance_v[vbal->m];
    double balance_amp;*/

  /* first things first.  Make sure encode is ready*/
  _oggpack_reset(opb);
  /* Encode the packet type */
  _oggpack_write(opb,0,1);

  /* Encode the block size */
  _oggpack_write(opb,vb->W,1);
  if(vb->W){
    _oggpack_write(opb,vb->lW,1);
    _oggpack_write(opb,vb->nW,1);
  }

  /* No envelope encoding yet */
  _oggpack_write(opb,0,1);
  
  /* time domain PCM -> MDCT domain */
  for(i=0;i<vi->channels;i++)
    mdct_forward(&vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);

  /* no balance yet */
    
  /* extract the spectral envelope and residue */
  /* just do by channel.  No coupling yet */
  {
    for(i=0;i<vi->channels;i++){
      static int frameno=0;
      int j;
      double *floor=alloca(n/2*sizeof(double));
      double *curve=alloca(n/2*sizeof(double));
      double *lpc=vb->lpc[i];
      double *lsp=vb->lsp[i];

      memset(floor,0,sizeof(double)*n/2);
      
#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"Aspectrum%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",vb->pcm[i][j]);
	fclose(out);

      }
#endif

      _vp_mask_floor(vp,vb->pcm[i],floor);

#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"Apremask%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",floor[j]);
	fclose(out);
      }
#endif

      /* Convert our floor to a set of lpc coefficients */
      vb->amp[i]=sqrt(vorbis_curve_to_lpc(floor,lpc,vl));

      /* LSP <-> LPC is orthogonal and LSP quantizes more stably */
      vorbis_lpc_to_lsp(lpc,lsp,vl->m);

      /* code the spectral envelope; mutates the lsp coeffs to reflect
         what was actually encoded */
      _vs_spectrum_encode(vb,vb->amp[i],lsp);

      /* Generate residue from the decoded envelope, which will be
         slightly different to the pre-encoding floor due to
         quantization.  Slow, yes, but perhaps more accurate */

      vorbis_lsp_to_lpc(lsp,lpc,vl->m); 
      vorbis_lpc_to_curve(curve,lpc,vb->amp[i],vl);

      /* this may do various interesting massaging too...*/
      if(vb->amp[i])_vs_residue_train(vb,vb->pcm[i],curve,n/2);
      _vs_residue_quantize(vb->pcm[i],curve,vi,n/2);

#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"Alpc%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<vl->m;j++)
	  fprintf(out,"%g\n",lpc[j]);
	fclose(out);

	sprintf(buffer,"Alsp%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<vl->m;j++)
	  fprintf(out,"%g\n",lsp[j]);
	fclose(out);

	sprintf(buffer,"Amask%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",curve[j]);
	fclose(out);

	sprintf(buffer,"Ares%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",vb->pcm[i][j]);
	fclose(out);
      }
#endif

      /* encode the residue */
      _vs_residue_encode(vb,vb->pcm[i]);

    }
  }

  /* set up the packet wrapper */

  op->packet=opb->buffer;
  op->bytes=_oggpack_bytes(opb);
  op->b_o_s=0;
  op->e_o_s=vb->eofflag;
  op->frameno=vb->frameno;
  op->packetno=vb->sequence; /* for sake of completeness */

  return(0);
}




/* commented out, relocated balance stuff */
  /*{
    double *C=vb->pcm[0];
    double *D=vb->pcm[1];
    
    balance_amp=_vp_balance_compute(D,C,balance_v,vbal);
    
    {
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"com%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
        fprintf(out," 0. 0.\n");
	fprintf(out,"%g %g\n",C[i],D[i]);
	fprintf(out,"\n");
      }
      fclose(out);
      
      sprintf(buffer,"L%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",C[i]);
      }
      fclose(out);
      sprintf(buffer,"R%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",D[i]);
      }
      fclose(out);
      
    }
    
    _vp_balance_apply(D,C,balance_v,balance_amp,vbal,1);
      
    {
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"bal%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out," 0. 0.\n");
	fprintf(out,"%g %g\n",C[i],D[i]);
	fprintf(out,"\n");
      }
      fclose(out);
      sprintf(buffer,"C%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",C[i]);
      }
      fclose(out);
      sprintf(buffer,"D%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",D[i]);
      }
      fclose(out);
      
    }
  }*/
