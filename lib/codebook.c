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

 function: basic codebook pack/unpack/code/decode operations
 last mod: $Id: codebook.c,v 1.2 2000/01/12 11:16:34 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/codebook.h"
#include "bitwise.h"
#include "bookinternal.h"

/**** pack/unpack helpers ******************************************/
static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static long _float24_pack(double val){
  int sign=0;
  long exp;
  long mant;
  if(val<0){
    sign=0x800000;
    val= -val;
  }
  exp= floor(log(val)/log(2));
  mant=rint(ldexp(val,17-exp));
  exp=(exp+VQ_FEXP_BIAS)<<18;

  return(sign|exp|mant);
}

static double _float24_unpack(long val){
  double mant=val&0x3ffff;
  double sign=val&0x800000;
  double exp =(val&0x7c0000)>>18;
  if(sign)mant= -mant;
  return(ldexp(mant,exp-17-VQ_FEXP_BIAS));
}

/* given a list of word lengths, generate a list of codewords.  Works
   for length ordered or unordered, always assigns the lowest valued
   codewords first */
long *_make_words(long *l,long n){
  long i,j;
  long marker[33];
  long *r=malloc(n*sizeof(long));
  memset(marker,0,sizeof(marker));

  for(i=0;i<n;i++){
    long length=l[i];
    long entry=marker[l[i]];

    /* when we claim a node for an entry, we also claim the nodes
       below it (pruning off the imagined tree that may have dangled
       from it) as well as blocking the use of any nodes directly
       above for leaves */

    /* update ourself */
    if(length<32 && (entry>>length)){
      /* error condition; the lengths must specify an overpopulated tree */
      free(r);
      return(NULL);
    }
    r[i]=entry;
    
    /* Look to see if the next shorter marker points to the node
       above. if so, update it and repeat.  */
    {
      for(j=length;j>0;j--){

	if(marker[j]&1){
	  /* have to jump branches */
	  if(j==1)
	    marker[1]++;
	  else
	    marker[j]=marker[j-1]<<1;
	  break; /* invariant says next upper marker would already
		    have been moved if it was on the same path */
	}
	marker[j]++;
      }
    }

    /* prune the tree; the implicit invariant says all the longer
       markers were dangling from our just-taken node.  Dangle them
       from our *new* node. */
    for(j=length+1;j<33;j++)
      marker[j]=marker[j-1]<<1;
  }

  /* bitreverse the words because our bitwise packer/unpacker is LSb
     endian */
  for(i=0;i<n;i++){
    long temp=0;
    for(j=0;j<l[i];j++){
      temp<<=1;
      temp|=(r[i]>>j)&1;
    }
    r[i]=temp;
  }

  return(r);
}

/* unpack the quantized list of values for encode/decode ***********/
static double *_book_unquantize(codebook *b){
  long j,k;
  double mindel=_float24_unpack(b->q_min);
  double delta=_float24_unpack(b->q_delta);
  double *r=malloc(sizeof(double)*b->entries*b->dim);

  for(j=0;j<b->entries;j++){
    double last=0.;
    for(k=0;k<b->dim;k++){
      double val=b->quantlist[j*b->dim+k]*delta+last+mindel;
      r[j*b->dim+k]=val;
      if(b->q_sequencep)last=val;
    }
  }
  return(r);
}

/**** Defend the abstraction ****************************************/

/* some elements in the codebook structure are assumed to be pointers
   to static/shared storage (the pointers are duped, and free below
   does not touch them.  The fields are unused by decode):

   quantlist,
   lengthlist,
   encode_tree

*/ 

void vorbis_book_dup(codebook *dest,const codebook *source){
  long entries=source->entries;
  long dim=source->dim;
  memcpy(dest,source,sizeof(codebook));

  /* handle non-flat storage */
  if(source->valuelist){
    dest->valuelist=malloc(sizeof(double)*dim*entries);
    memcpy(dest->valuelist,source->valuelist,sizeof(double)*dim*entries);
  }
  if(source->codelist){
    dest->codelist=malloc(sizeof(long)*entries);
    memcpy(dest->codelist,source->codelist,sizeof(long)*entries);
  }

  /* encode tree is assumed to be static storage; don't free it */

  if(source->decode_tree){
    long aux=source->decode_tree->aux;
    dest->decode_tree=malloc(sizeof(decode_aux));
    dest->decode_tree->aux=aux;
    dest->decode_tree->ptr0=malloc(sizeof(long)*aux);
    dest->decode_tree->ptr1=malloc(sizeof(long)*aux);

    memcpy(dest->decode_tree->ptr0,source->decode_tree->ptr0,sizeof(long)*aux);
    memcpy(dest->decode_tree->ptr1,source->decode_tree->ptr1,sizeof(long)*aux);
  }
}

void vorbis_book_clear(codebook *b){
  if(b->decode_tree){
    free(b->decode_tree->ptr0);
    free(b->decode_tree->ptr1);
    memset(b->decode_tree,0,sizeof(decode_aux));
    free(b->decode_tree);
  }
  if(b->valuelist)free(b->valuelist);
  if(b->codelist)free(b->codelist);
  memset(b,0,sizeof(codebook));
}

/* packs the given codebook into the bitstream
   side effects: populates the valuelist and codeword members ***********/
int vorbis_book_pack(codebook *c,oggpack_buffer *b){
  long i,j;
  int ordered=0;

  /* first the basic parameters */
  _oggpack_write(b,0x564342,24);
  _oggpack_write(b,c->dim,16);
  _oggpack_write(b,c->entries,24);

  /* pack the codewords.  There are two packings; length ordered and
     length random.  Decide between the two now. */
  
  for(i=1;i<c->entries;i++)
    if(c->lengthlist[i]<c->lengthlist[i-1])break;
  if(i==c->entries)ordered=1;
  
  if(ordered){
    /* length ordered.  We only need to say how many codewords of
       each length.  The actual codewords are generated
       deterministically */

    long count=0;
    _oggpack_write(b,1,1);  /* ordered */
    _oggpack_write(b,c->lengthlist[0]-1,5); /* 1 to 32 */

    for(i=1;i<c->entries;i++){
      long this=c->lengthlist[i];
      long last=c->lengthlist[i-1];
      if(this>last){
	for(j=last;j<this;j++){
	  _oggpack_write(b,i-count,ilog(c->entries-count));
	  count=i;
	}
      }
    }
    _oggpack_write(b,i-count,ilog(c->entries-count));

  }else{
    /* length random.  Again, we don't code the codeword itself, just
       the length.  This time, though, we have to encode each length */
    _oggpack_write(b,0,1);   /* unordered */
    for(i=0;i<c->entries;i++)
      _oggpack_write(b,c->lengthlist[i]-1,5);
  }

  /* is the entry number the desired return value, or do we have a
     mapping? */
  if(c->quantlist){
    /* we have a mapping.  bundle it out. */
    _oggpack_write(b,1,1);

    /* values that define the dequantization */
    _oggpack_write(b,c->q_min,24);
    _oggpack_write(b,c->q_delta,24);
    _oggpack_write(b,c->q_quant-1,4);
    _oggpack_write(b,c->q_sequencep,1);

    /* quantized values */
    for(i=0;i<c->entries*c->dim;i++)
      _oggpack_write(b,c->quantlist[i],c->q_quant);

  }else{
    /* no mapping. */
    _oggpack_write(b,0,1);
  }

  c->codelist=_make_words(c->lengthlist,c->entries);
  c->valuelist=_book_unquantize(c);
  
  return(0);
}

/* unpacks a codebook from the packet buffer into the codebook struct,
   readies the codebook auxiliary structures for decode *************/
int vorbis_book_unpack(oggpack_buffer *b,codebook *c){
  long i,j;
  memset(c,0,sizeof(codebook));

  /* make sure alignment is correct */
  if(_oggpack_read(b,24)!=0x564342)goto _eofout;

  /* first the basic parameters */
  c->dim=_oggpack_read(b,16);
  c->entries=_oggpack_read(b,24);
  if(c->entries==-1)goto _eofout;

  /* codeword ordering.... length ordered or unordered? */
  switch(_oggpack_read(b,1)){
  case 0:
    /* unordered */
    c->lengthlist=malloc(sizeof(long)*c->entries);
    for(i=0;i<c->entries;i++){
      long num=_oggpack_read(b,5);
      if(num==-1)goto _eofout;
      c->lengthlist[i]=num+1;
    }

    break;
  case 1:
    /* ordered */
    {
      long length=_oggpack_read(b,5)+1;
      c->lengthlist=malloc(sizeof(long)*c->entries);

      for(i=0;i<c->entries;){
	long num=_oggpack_read(b,ilog(c->entries-i));
	if(num==-1)goto _eofout;
	for(j=0;j<num;j++,i++)
	  c->lengthlist[i]=length;
	length++;
      }
    }
    break;
  default:
    /* EOF */
    return(-1);
  }
  
  /* now we generate the codewords for the given lengths */
  c->codelist=_make_words(c->lengthlist,c->entries);
  if(c->codelist==NULL)goto _errout;

  /* ...and the decode helper tree from the codewords */
  {
    long top=0;
    decode_aux *t=c->decode_tree=malloc(sizeof(decode_aux));
    long *ptr0=t->ptr0=calloc(c->entries*2,sizeof(long));
    long *ptr1=t->ptr1=calloc(c->entries*2,sizeof(long));
    t->aux=c->entries*2;

    for(i=0;i<c->entries;i++){
      long ptr=0;
      for(j=0;j<c->lengthlist[i]-1;j++){
	int bit=(c->codelist[i]>>j)&1;
	if(!bit){
	  if(!ptr0[ptr])
	    ptr0[ptr]= ++top;
	  ptr=ptr0[ptr];
	}else{
	  if(!ptr1[ptr])
	    ptr1[ptr]= ++top;
	  ptr=ptr1[ptr];
	}
      }
      if(!((c->codelist[i]>>j)&1))
	ptr0[ptr]=-i;
      else
	ptr1[ptr]=-i;
    }
  }
  /* no longer needed */
  free(c->lengthlist);c->lengthlist=NULL;
  free(c->codelist);c->codelist=NULL;

  /* Do we have a mapping to unpack? */
  if(_oggpack_read(b,1)){

    /* values that define the dequantization */
    c->q_min=_oggpack_read(b,24);
    c->q_delta=_oggpack_read(b,24);
    c->q_quant=_oggpack_read(b,4)+1;
    c->q_sequencep=_oggpack_read(b,1);

    /* quantized values */
    c->quantlist=malloc(sizeof(double)*c->entries*c->dim);
    for(i=0;i<c->entries*c->dim;i++)
      c->quantlist[i]=_oggpack_read(b,c->q_quant);
    if(c->quantlist[i-1]==-1)goto _eofout;
    c->valuelist=_book_unquantize(c);
    free(c->quantlist);c->quantlist=NULL;
  }

  /* all set */
  return(0);

 _errout:
 _eofout:
  if(c->lengthlist)free(c->lengthlist);c->lengthlist=NULL;
  if(c->quantlist)free(c->quantlist);c->quantlist=NULL;
  vorbis_book_clear(c);
  return(-1);
 
}

/* returns the number of bits ***************************************/
int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b){
  _oggpack_write(b,book->codelist[a],book->lengthlist[a]);
  return(book->lengthlist[a]);
}

/* returns the number of bits and *modifies a* to the entry value *****/
int vorbis_book_encodev(codebook *book, double *a, oggpack_buffer *b){
  encode_aux *t=book->encode_tree;
  int dim=book->dim;
  int ptr=0,k;

  while(1){
    double c=0.;
    double *p=book->valuelist+t->p[ptr];
    double *q=book->valuelist+t->q[ptr];
    
    for(k=0;k<dim;k++)
      c+=(p[k]-q[k])*(a[k]-(p[k]+q[k])*.5);

    if(c>0.) /* in A */
      ptr= -t->ptr0[ptr];
    else     /* in B */
      ptr= -t->ptr1[ptr];
    if(ptr<=0)break;
  }
  memcpy(a,book->valuelist-ptr*dim,dim*sizeof(double));
  return(vorbis_book_encode(book,-ptr,b));
}

/* returns the entry number or -1 on eof ****************************/
long vorbis_book_decode(codebook *book, oggpack_buffer *b){
  long ptr=0;
  decode_aux *t=book->decode_tree;
  do{
    switch(_oggpack_read1(b)){
    case 0:
      ptr=t->ptr0[ptr];
      break;
    case 1:
      ptr=t->ptr1[ptr];
      break;
    case -1:
      return(-1);
    }
  }while(ptr>0);
  return(-ptr);
}

/* returns the entry number or -1 on eof ****************************/
long vorbis_book_decodev(codebook *book, double *a, oggpack_buffer *b){
  long entry=vorbis_book_decode(book,b);
  if(entry==-1)return(-1);
  memcpy(a,book->valuelist+entry*book->dim,sizeof(double)*book->dim);
  return(0);
}

#ifdef _V_SELFTEST

/* Simple enough; pack a few candidate codebooks, unpack them.  Code a
   number of vectors through (keeping track of the quantized values),
   and decode using the unpacked book.  quantized version of in should
   exactly equal out */

#include <stdio.h>
#include "vorbis/book/lsp20_0.vqh"
#include "vorbis/book/lsp32_0.vqh"
#define TESTSIZE 40
#define TESTDIM 4

double test1[40]={
  0.105939,
  0.215373,
  0.429117,
  0.587974,

  0.181173,
  0.296583,
  0.515707,
  0.715261,

  0.162327,
  0.263834,
  0.342876,
  0.406025,

  0.103571,
  0.223561,
  0.368513,
  0.540313,

  0.136672,
  0.395882,
  0.587183,
  0.652476,

  0.114338,
  0.417300,
  0.525486,
  0.698679,

  0.147492,
  0.324481,
  0.643089,
  0.757582,

  0.139556,
  0.215795,
  0.324559,
  0.399387,

  0.120236,
  0.267420,
  0.446940,
  0.608760,

  0.115587,
  0.287234,
  0.571081,
  0.708603,
};

double test2[40]={
  0.088654,
  0.165742,
  0.279013,
  0.395894,

  0.110812,
  0.218422,
  0.283423,
  0.371719,

  0.136985,
  0.186066,
  0.309814,
  0.381521,

  0.123925,
  0.211707,
  0.314771,
  0.433026,

  0.088619,
  0.192276,
  0.277568,
  0.343509,

  0.068400,
  0.132901,
  0.223999,
  0.302538,

  0.202159,
  0.306131,
  0.360362,
  0.416066,

  0.072591,
  0.178019,
  0.304315,
  0.376516,

  0.094336,
  0.188401,
  0.325119,
  0.390264,

  0.091636,
  0.223099,
  0.282899,
  0.375124,
};

codebook *testlist[]={&_vq_book_lsp20_0,&_vq_book_lsp32_0,NULL};
double   *testvec[]={test1,test2};

int main(){
  oggpack_buffer write;
  oggpack_buffer read;
  long ptr=0,i;
  _oggpack_writeinit(&write);
  
  fprintf(stderr,"Testing codebook abstraction...:\n");

  while(testlist[ptr]){
    codebook c;
    double *qv=alloca(sizeof(double)*TESTSIZE);
    double *iv=alloca(sizeof(double)*TESTSIZE);
    memcpy(qv,testvec[ptr],sizeof(double)*TESTSIZE);

    fprintf(stderr,"\tpacking/coding %ld... ",ptr);

    /* pack the codebook, write the testvector */
    _oggpack_reset(&write);
    vorbis_book_dup(&c,testlist[ptr]); /* get it into memory we can write */
    vorbis_book_pack(&c,&write);
    fprintf(stderr,"Codebook size %ld bytes... ",_oggpack_bytes(&write));
    for(i=0;i<TESTSIZE;i+=TESTDIM)
      vorbis_book_encodev(&c,qv+i,&write);
    vorbis_book_clear(&c);

    fprintf(stderr,"OK.\n");
    fprintf(stderr,"\tunpacking/decoding %ld... ",ptr);

    /* transfer the write data to a read buffer and unpack/read */
    _oggpack_readinit(&read,_oggpack_buffer(&write),_oggpack_bytes(&write));
    if(vorbis_book_unpack(&read,&c)){
      fprintf(stderr,"Error unpacking codebook.\n");
      exit(1);
    }
    for(i=0;i<TESTSIZE;i+=TESTDIM)
      if(vorbis_book_decodev(&c,iv+i,&read)){
	fprintf(stderr,"Error reading codebook test data (EOP).\n");
	exit(1);
      }
    for(i=0;i<TESTSIZE;i++)
      if(qv[i]!=iv[i]){
	fprintf(stderr,"input (%g) != output (%g) at position (%ld)\n",
		iv[i],qv[i],i);
	exit(1);
      }
	  
    fprintf(stderr,"OK\n");
    ptr++;
  }
  exit(0);
}

#endif
