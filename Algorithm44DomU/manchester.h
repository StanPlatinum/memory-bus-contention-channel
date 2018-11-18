#include<stdio.h>
#include<string.h>
#include<stdlib.h>

void diff_encode (char *str0, char *Dif_Manch, int M)
{ 
	int i,j=2;
	memset(Dif_Manch, 0, 2*M);
	Dif_Manch[0] = 0; 
	Dif_Manch[1] = 1;
	if (str0[0]){
		Dif_Manch[0] = 1; 
		Dif_Manch[1] = 0;
	} 
	//for (i=0;i<10;i++) printf("%c",Dif_Manch[i]);
	//printf("\n");

	for (i = 1; i <M; i++) 
	{ 

		if (!str0[i]) 
		{ 
			Dif_Manch[j++] = Dif_Manch[j - 3]; 
			Dif_Manch[j++] = Dif_Manch[j - 3]; 
		} 
		else
		{ 
			Dif_Manch[j++] = Dif_Manch[j - 2]; 
			Dif_Manch[j++] = Dif_Manch[j - 4]; 
		}  
	}
} 

int diff_decode(unsigned char *seg, unsigned char *sample, float T,int M, int bg){
	int i=1,j=0,p=0,q=0,s=1,mark ,count = 0,flag = 0,flag1=0,flag2=0;
	int *index = malloc(2*M*sizeof(int)); 
	unsigned char *buf = malloc(2*M);
	i = bg+(int)T/2+1;
	mark = sample[i];

	while(p<M*2){
		i++;
		if(sample[i]!=mark){

			if(count>T/2) buf[p++] = mark;

			mark = sample[i]; 
			count = 1;
		}
		else{
			count++;
			if(count==T) {buf[p++] = mark;count = 0;} 
		}
	}
	seg[0] = buf[0];
	for(i=1;i<2*M;i+=2){
		if(buf[i-1]==buf[i]) {pause();return 1;}
		seg[++q] = buf[i+1] == buf[i];
	}
	return 0;
}

/*
   void diff_decode(unsigned char *str0, unsigned char *sample, int M, int bg, int end){
   int i=1,j=0,T=3,p=0,q=0,s=1,mark = 1,tag = 0;
   int index[2*M]; 
   unsigned char buf[M];

   for(i=bg+1;i<end-1;i++) {
   if(sample[i]!=sample[i-1]) index[p++] = i;
//if(p>1 && T > index[p-1] - index[p-2]) T = index[p-1] - index[p-2];
}

for(i=0;i<p;i++){

if(!tag){
if(mark)
str0[q++] = sample[index[i]];
else
str0[q++] = !sample[index[i]];

}
if(index[i+1] - index[i] < 2*T) {if(mark) tag = 1; else tag = 0;mark=!mark;} 
else if(index[i+1] - index[i] >= 3*T) {tag = 0;mark = !mark;}
else tag = 0;
}

// str0[0] = buf[0];
//for(i=1;i<q;i++) str0[s++] = (buf[i]!=buf[i-1]);
}
 */

/*
   int diff_decode(unsigned char *seg, unsigned char *sample, float T,int M, int bg){
   int i=1,j=0,p=0,q=0,s=1,mark ,count = 0;
   int *index = malloc(2*M*sizeof(int)); 
   unsigned char *buf = malloc(2*M);
   i = bg+(int)T/2+1;
   mark = sample[i];
   while(p<M*2){
   i++;
   if(sample[i]!=mark){
   if(count>T/2) buf[p++] = mark;
   mark = sample[i]; 
   count = 1;
   }
   else{
   count++;
   if(count==T) {buf[p++] = mark;count = 0;} 
   }
   }
   seg[0] = buf[0];
   for(i=1;i<2*M;i+=2){
   if(buf[i-1]==buf[i]) return 1;
   seg[++q] = buf[i+1] == buf[i];
   }
   return 0;
   }
 */

