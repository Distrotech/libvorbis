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

 function: simple example encoder
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Nov 16 1999

 ********************************************************************/

/* takes a stereo 16bit 44.1kHz WAV file from stdin and encodes it into
   a Vorbis bitstream */

/* Note that this is POSIX, not ANSI, code */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "codec.h"

#define READ 1024
signed char readbuffer[READ*4+44]; /* out of the data segment, not the stack */

int main(){
  ogg_stream_state os; /* take physical pages, weld into a logical
			  stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */
  
  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
			  settings */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

  int eos=0;

  /* we cheat on the WAV header; we just bypass 44 bytes and never
     verify that it matches 16bit/stereo/44.1kHz.  This is just an
     example, after all. */

  fread(readbuffer,1,44,stdin);

  /********** Encode setup ************/

  /* choose an encoding mode */
  /* (mode 0: 44kHz stereo uncoupled, roughly 128kbps VBR) */
  vorbis_info_modeset(&vi,0); 

  /* add a comment */
  vorbis_info_addcomment(&vi,"Track encoded by encoder_example.c");

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init(&vd,&vi);
  vorbis_block_init(&vd,&vb);
  
  /* set up our packet->stream encoder */
  /* pick a random serial number; that way we can more likely build
     chained streams just by concatenation */
  srandom(time(NULL));
  ogg_stream_init(&os,random());

  /* Vorbis streams begin with three headers; the initial header (with
     most of the codec setup parameters) which is mandated by the Ogg
     bitstream spec.  The second header holds any comment fields.  The
     third header holds the bitstream codebook.  We merely need to
     make the headers, then pass them to libvorbis one at a time;
     libvorbis handles the additional Ogg bitstream constraints */

  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_info_headerout(&vi,&header,&header_comm,&header_code);
    ogg_stream_packetin(&os,&header); /* automatically placed in its own
					 page */
    ogg_stream_packetin(&os,&header_comm);
    ogg_stream_packetin(&os,&header_code);

    /* no need to write out here.  We'll get to that in the main loop */
  }
  
  while(!eos){
    long i;
    long bytes=fread(readbuffer,1,READ*4,stdin); /* stereo hardwired here */

    if(bytes==0){
      /* end of file.  this can be done implicitly in the mainline,
         but it's easier to see here in non-clever fashion.
         Tell the library we're at end of stream so that it can handle
         the last frame and mark end of stream in the output properly */
      vorbis_analysis_wrote(&vd,0);

    }else{
      /* data to encode */

      /* expose the buffer to submit data */
      double **buffer=vorbis_analysis_buffer(&vd,READ);
      
      /* uninterleave samples */
      for(i=0;i<bytes/4;i++){
	buffer[0][i]=((readbuffer[i*4+1]<<8)|
		      (0x00ff&(int)readbuffer[i*4]))/32768.;
	buffer[1][i]=((readbuffer[i*4+3]<<8)|
		      (0x00ff&(int)readbuffer[i*4+2]))/32768.;
      }
    
      /* tell the library how much we actually submitted */
      vorbis_analysis_wrote(&vd,i);
    }

    /* vorbis does some data preanalysis, then divvies up blocks for
       more involved (potentially parallel) processing.  Get a single
       block for encoding now */
    while(vorbis_analysis_blockout(&vd,&vb)==1){

      /* analysis */
      vorbis_analysis(&vb,&op);

      /* weld the packet into the bitstream */
      ogg_stream_packetin(&os,&op);

      /* write out pages (if any) */
      while(!eos){
	int result=ogg_stream_pageout(&os,&og);
	if(result==0)break;
	fwrite(og.header,1,og.header_len,stdout);
	fwrite(og.body,1,og.body_len,stdout);

	/* this could be set above, but for illustrative purposes, I do
	   it here (to show that vorbis does know where the stream ends) */
	
	if(ogg_page_eos(&og))eos=1;

      }
    }
  }

  /* clean up and exit.  vorbis_info_clear() must be called last */
  
  ogg_stream_clear(&os);
  vorbis_dsp_clear(&vd);
  vorbis_block_clear(&vb);
  vorbis_info_clear(&vi);
  
  /* ogg_page and ogg_packet structs always point to storage in
     libvorbis.  They're never freed or manipulated directly */
  
  fprintf(stderr,"Done.\n");
  return(0);
}

