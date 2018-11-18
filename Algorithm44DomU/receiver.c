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

char *b;
unsigned char *sample;
int threshold = 4800,T = 300;
float S = 7;
atomic_t *v;

unsigned long long rdtsc_value()
{
	unsigned long lo, hi;
	asm volatile ( "rdtsc"
			: "=a"(lo), "=d"(hi));
	return (unsigned long long)((hi<<32)|(lo));
}

int diff_decode(unsigned char *seg, unsigned char *sample, float T,int M, int bg){
	int i=1,j=0,p=0,q=0,s=1,mark ,count = 0,flag = 0,flag1=0,flag2=0;
	int *index = malloc(2*M*sizeof(int));
	unsigned char *buf = malloc(2*M);
	i = bg+(int)T/2+1;
	mark = sample[i];

	/* for test */
	FILE * recd_test;
	char * recd_test_path = "./diff_decoding_test.txt";
	int ti = 0;

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

	/* for test */
	if((recd_test = fopen(recd_test_path, "at+")) == NULL) exit(1);
	fprintf(recd_test, "\n");

	seg[0] = buf[0];
	for(i=1;i<2*M;i+=2){
		/* for test */
		//fprintf(recd_test, "%d", (int)buf[i]);
		
		if(buf[i-1]==buf[i]) {
			
			fprintf(recd_test, "\nseg_error, q=%d\n", q);
			fclose(recd_test);
			return 1;
		}
		
		seg[++q] = buf[i+1] == buf[i];
		fprintf(recd_test, "%d", (int)seg[q]);
	}
	/* for test */
	fprintf(recd_test, "\nreceived successfully");
	fclose(recd_test);
	return 0;
}


atomic_t* unaligned(int line){
	void *p = malloc(2*line);
	while((long)p%4096  >= 3968){
		free(p);
		p = malloc(2*line);
	}
	atomic_t *v = (atomic_t *) (((long)p/line + 1)*line - sizeof(atomic_t)/2);
	return v;
}


int bin2char(char *ch, char *bin){
	int i,sum=0,x=0;
	for(i=0;i<8;i++) {x = 2*x + bin[i];sum += bin[i];}
	*ch = (char)x;
	return sum==0;
}

int receiveframe(int *bg, int *end){

	memset(sample,0,4096);
	int res = 0,con =0;
	while(!(res = receivebits(bg,end))); //if res == 1, start receiving the frame
	printf("%d,%d",*bg,*end);
	res = receivebits(bg,end);

	return 0;
}

char * filename_coloring;
char * nameappend;

int main(int argc, char **argv){
	unsigned char *msg = calloc(223*8,sizeof(char));
	unsigned char *recd = calloc(255,sizeof(char));
	char ch;
	FILE * fp;
	char * received_path = "./recd.txt";
	int mj=0,i,j,x=0,bg=0,end=0,res=0,frm=0,zeros=0;

	sample = calloc(4096,sizeof(char));


	b = malloc(64);
	v=unaligned(64);
	sscanf(argv[1],"%d",&threshold);
	sscanf(argv[2],"%d",&T);
	sscanf(argv[3],"%f",&S);

	/* for test */
	nameappend = (char *)malloc(10);
	sprintf(nameappend, "%d", T);
	filename_coloring = (char *)malloc(25);
	strcpy(filename_coloring, "inv_data_T_");
	strcat(filename_coloring, nameappend);
	/* the length of filename must be less than 25 */

	generate_gf();
	gen_poly();

	while(!res){

		while(1){
			bg=0;end=0;
			res = receiveframe(&bg,&end);
			if(!res) {
				res = diff_decode(recd, sample, S, 255, bg);
			}
			else break;
			if(res) {printf("\nError!Retry...Frame%d...\n",frm);sleep(1);} else {printf("OK!Frame%d\n",frm);break;}
		}

		if(!res){
			decode_rs(&msg[mj],recd);
			printf("\nReceived Frame%d:\n",frm++);
			for(j=0;j<223;j++) if(msg[mj+j]) putchar('+'); else putchar('-');
			printf("\n");
			mj += 223;
		}
		else printf("\nMission Complete!\n");

		if(mj==223*8){
			printf("start to write the data into file...\n");
			for(i=0;i<mj;i+=8) {
				if((fp = fopen(received_path, "at+")) == NULL) exit(1);
				if(res = bin2char(&ch, &msg[i])){
					zeros++;
					if(zeros == 9) {
						res = 1;
						printf("transform err or finished.\n");
						break;
					}
				}
				else { 
					putchar(ch); 
					fprintf(fp, "%c", ch);
				}
				fclose(fp);
			}
			mj=0;zeros=0;
		}
	}
	return 0;
}

int receivebits(int *bg,int *ed){
	unsigned long long start, end;
	long inv = 0,latency;
	int i,j,count0=0,count1=0;
	int is_save = 1;//eliminate unnecessary 0

	/* designate the output file of result coloring */
	FILE * outputFile;
	outputFile = fopen(filename_coloring, "at+");
	fprintf(outputFile, "\n");
	int color;

	for(i=*bg;i<4096;i++){
		inv = 0;
		atomic_set(v,T);
		start = rdtsc_value();
		while(!atomic_dec_and_test(v));
		end = rdtsc_value();
		inv = end - start;
		/* for test */
		color = -1;

		if(inv/T > threshold) {
			putchar('+');
			sample[i] = 1;
			count1++;
			if(count0==1) sample[i-1] = sample[i-2];
			if (count0>S*4) {*ed = i;return 0;}
			count0 = 0;
			is_save = 1;
			/* for test */
			color = 1;
		}
		else{
			//putchar('-');
			if(is_save) {
				sample[i] = 0;
				count0++;
				putchar('-');
				if(count1==1) sample[i-1] = sample[i-2];
				if (count1>S*5) {*bg = i;return 1;}
				count1 = 0;
			}
			else {
				i--;
				//putchar('*');
			}
			is_save = !is_save;
			/* for test */
			color = 0;
		}
		/* write the inv data to inv_data txt file */	
		if ( inv>0 && inv<10000000 && color>-1) {
			fprintf(outputFile, "%d\t%ld\t\t%d\n", i, inv, color);
		}
	}
	/* for test */
	fclose(outputFile);
	return 0;
}

