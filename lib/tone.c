#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

void usage(){
  fprintf(stderr,"tone <frequency_Hz>,[<amplitude>] [<frequency_Hz>,[<amplitude>]...]\n");
  exit(1);
}

int main (int argc,char *argv[]){
  int i,j;
  double *f;
  double *amp;
  
  if(argc<2)usage();

  f=alloca(sizeof(float)*(argc-1));
  amp=alloca(sizeof(float)*(argc-1));

  i=0;
  while(argv[i+1]){
    char *pos=strchr(argv[i+1],',');
    
    f[i]=atof(argv[i+1]);
    if(pos)
      amp[i]=atof(pos+1)*32767.;
    else
      amp[i]=32767.;

    i++;
  }

  for(i=0;i<44100*10;i++){
    float val=0;
    int ival;
    for(j=0;j<argc-1;j++)
      val+=amp[j]*sin(i/44100.*f[j]*2*M_PI);
    ival=rint(val);

    if(ival>32767.)ival=32767.;
    if(ival<-32768.)ival=-32768.;

    fprintf(stdout,"%c%c%c%c",
	    (char)(ival&0xff),
	    (char)((ival>>8)&0xff),
	    (char)(ival&0xff),
	    (char)((ival>>8)&0xff));
  }
  return(0);
}

