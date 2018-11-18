#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#include <emmintrin.h>
#include <string.h>

#include "my_atomic.h"
#include "rs.h"
#include "manchester.h"

unsigned char frmHead[12] = {1,1,1,1,1,1,1,1,1,1,1,0}, frmFoot[8] = {0,0,0,0,0,0,0,1};
unsigned char sys[32];

char *b;
int opt,threshold = 4200,T,n;
extern char *optarg;
atomic_t *v;

atomic_t* unaligned(int line){
	void *p = malloc(2*line);
	while((long)p%4096  >= 3968){
		free(p);
		p = malloc(2*line);
	}
	atomic_t *v = (atomic_t *) (((long)p/line + 1)*line -sizeof(atomic_t)/2);
	return v;
}

void setbytes(char *p, char c){
	__m128i i = _mm_set_epi8(c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c);

	_mm_stream_si128((__m128i *)&p[0], i);
	_mm_stream_si128((__m128i *)&p[16], i);
	_mm_stream_si128((__m128i *)&p[32], i);
	_mm_stream_si128((__m128i *)&p[48], i);
}

unsigned long long rdtsc_value()
{
	unsigned long lo, hi;
	asm volatile ( "rdtsc"
			: "=a"(lo), "=d"(hi));
	return (unsigned long long) ((hi<<32)|(lo));
}

int bin2char(char *ch, char *bin){
	int i,sum=0,x=0;
	for(i=0;i<8;i++) {x = 2*x + bin[i];sum += bin[i];}
	*ch = (char)x;
	return sum==0;
}

void char2bin(char ch, unsigned char *bin){
	int i;
	for(i=0;i<8;i++){
		if(ch & 0x80) bin[i]=1;else bin[i]=0;
		ch <<=1;
	}
}

int sendframe(unsigned char *seg){
	int result = 1;
	int result1 = 1;
	int result2 = 1;
	int result3 = 1;
	int result4 = 1;
	result = sendbits(sys,32);
	if(!result){
		result1 = sendbits(frmHead,12);
		result2 = sendbits(seg,255*2);//param 'threshold' used in here
		result3 = sendbits((char *)frmFoot,8);
	}
	if(!result1 && !result2 && !result3) result4 = sendbits(sys,32);//return 0 means successful
	return result4;
}

int sendbits(unsigned char *seg, int l){
	unsigned long long start, end;
	double inv = 0,latency=0;
	int i,j,count0=0;
#if 0
	/* designate the output file */
	FILE * outputFile;
	outputFile = fopen(filename, "at+");
	fprintf(outputFile, "\n");
#endif
	int color;

	for(i=0;i<l;i++){
		inv=0;
		/* for test */
		color = -1;

		if(seg[i]){
			atomic_set(v,T);
			start = rdtsc_value();
			//clock_gettime(CLOCK_MONOTONIC,&start);

			while(!atomic_dec_and_test(v));

			//clock_gettime(CLOCK_MONOTONIC,&end);
			end = rdtsc_value();
			//inv = end.tv_nsec-start.tv_nsec;
			inv = end - start;
			/* for test */
			color = 1;

			putchar('+');
		}
		else{
			count0=0;
			start = rdtsc_value();
			for(j=0;j<n;j++){
				count0++;
				//clock_gettime(CLOCK_MONOTONIC,&start);

				setbytes(b,'a');

				//clock_gettime(CLOCK_MONOTONIC,&end);
				//inv += end.tv_nsec-start.tv_nsec;
			}
			end = rdtsc_value();
			inv = end - start;
			/* for test */
			color = 0;

			//if(inv>0) latency += inv/count0;
			putchar('-');
		}//end else
#if 0
		/* write the inv data to sender_T txt file */	
		if ( inv>0 && inv<100000000 && color>-1 ) {
			fprintf(outputFile, "%d\t%ld\t\t%d\n", i, (long int)inv, color);
		}
#endif		
		if ( inv>0 && inv<100000000 ) latency += inv/T;
	}//end for
#if 0
	/* for test */
	fclose(outputFile);
#endif
	printf("\n");
	//printf("\nlatency/l=%d", (int)(latency/l));
	return (latency/l)<threshold;
}//end sendbits

int main(int argc, char **argv){
	unsigned char *msg = calloc(223*8,sizeof(char));
	unsigned char *seg = calloc(255*2,sizeof(char));
	unsigned char *recd = calloc(255,sizeof(char));

	char * sourcefile_path = "./sed_test.txt";
	register int i,j=0;
	int ml=0,frmi=0,mj=0,result=1,ms=223*8,mark=0,l,retryi=0;
	FILE * fp;
	char ch;
	b = malloc(64);
	v=unaligned(64);
	for(i=0;i<32;i++) sys[i] = !(i%2);

	sscanf(argv[1],"%d",&T);
	//n = T/2.4;
	//n = T*1.8;
	//n = T*1.35;
	//n = T*48;
	n = T*63;

	/*
	while((opt = getopt(argc, argv, "t:o:i:")) != EOF){
		switch(opt){
			case 't':
			sscanf(optarg,"%ld",&threshold);break;
			case 'o':
			sscanf(optarg,"%ld",&base0);break;
			case 'i':
			sscanf(optarg,"%ld",&base1);break;
		}sendbits
	}
	*/
#if 0
	/* for test */
	nameappend = (char *)malloc(10);
	sprintf(nameappend, "%d", T);
	filename = (char *)malloc(25);
	strcpy(filename, "sender_T_");
	strcat(filename, nameappend);
	/* the length of filename must be less than 25 */
#endif
	generate_gf();
	gen_poly();

	if((fp = fopen(sourcefile_path, "r")) == NULL) exit(1);

	while(!mark){

		if((ch = fgetc(fp)) == EOF) mark = 1;
		else {char2bin(ch,&msg[ml]);ml += 8;}
		if(ml==ms || mark){ 
			for(i=0;i<8;i++){
				encode_rs(&msg[mj], recd);
				diff_encode(recd,seg,255);

				result = 1;
				retryi=0;
				while(result){
					retryi++;
					result = sendframe(seg);
					if(retryi > 32) {printf("Mission abort!");break;}
					if(result) {printf("Retry ...Frame%d... \n\n",i+frmi*8);sleep(2);}
					else{
						printf("OK! Frame%d\n\n",i+frmi*8);
					}
				}
				printf("\n");
				mj += 223;
			}//end for
			mj=0;ml = 0;frmi++;
			memset(msg,0,223*8);
		}//end if
	}
	fclose(fp);

	return 0;
}
