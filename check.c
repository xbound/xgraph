#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "expr.h"
#include <time.h>
#include <err.h>
const struct proj {
	const char *e;
	double expect;
} projs[]={
	{"+7",7},
	{"-7",-7},
	{"1+5",6},
	{"5+3*8",29},
	{"(5+3)*8",64},
	{"134|45.5&7.5",135.5},
	{"134|(45.5&7.5)",135.5},
	{"(134|45.5)&7.5",7.5},
	{"6^3",216},
	{"6^^3",5},
	{"0&&1",0},
	{"1&&0",0},
	{"0&&0",0},
	{"1&&1",1},
	{"0||1",1},
	{"1||0",1},
	{"0||0",0},
	{"1||1",1},
	{"0^^^1",1},
	{"1^^^0",1},
	{"0^^^0",0},
	{"1^^^1",0},
	{"5-7+6",4},
	{"5-(7+6)",-8},
	{"5<<3>>2",10},
	{"5<<(3>>2)",5},
	{"(5<<3)>>2",10},
	{"5+3>>2",2},
	{"5>>3+2",5.0/32},
	{"drand48()-->x,x+2^x<3",1},
	{"drand48()-->x,x+2^x>=3",0},
	{"drand48()-->x,x+2^x>=0",1},
	{"drand48()-->x,x+2^x<0",0},
	{"!(5-2)",0},
	{"!(5-5)",1},
	{"!!(5-2)",1},
	{"!!(5-5)",0},
	{"med({0..10000})",5000},
	{"vmd(n,0,10000,1,n,med,0)",5000},
	{"sum(n,1,100,1,n)",5050},
	{"if(3,5,7)",5},
	{"if(0,5,7)",7},
	{"0-->m,while(m<7626,(1+m)->m,m)",7626},
	{"0-->m,while(m<=7626,(1+m)->m,m)",7627},
	{"-!3",-0.0},
	{"!-3",0},
	{"!-~3",0},
	{"!~-3",0},
	{"-!~3",-0.0},
	{"!!3",1},
	{"!!!3",0},
	{"!!0",0},
	{"!!!0",1},
	{"!~!-!!t",0},
	{"!!~!-!!t",1},
	{"(129+127)&~127",256},
	{"(128+127)&~127",128},
	{"(127+127)&~127",128},
	{NULL}

};
const struct eproj {
	const char *e;
	int expect;
} eprojs[]={
	{"2+hshdjxjdjxhxhx",EXPR_ESYMBOL},
	{"xor(2,3,5,gg,8)",EXPR_ESYMBOL},
	{"5->sum",EXPR_ESYMBOL},
	{"sum(",EXPR_EPT},
	{"sum",EXPR_EFP},
	{"sqrt()",EXPR_ENVP},
	{"sum(n)",EXPR_ENEA},
	{"266j",EXPR_ENUMBER},
	{"5->pi",EXPR_ETNV},
	{"5->sqrt",EXPR_ETNV},
	{"5->t",EXPR_ETNV},
	{"2+",EXPR_EEV},
	{"2->",EXPR_EEV},
	{"2->+",EXPR_EUO},
	{"drand48(2)",EXPR_EZAFP},
	{"drand48(",EXPR_EZAFP},
	{"drand48",EXPR_EZAFP},
	{"5-->pi",EXPR_EDS},
	{"5-->t",EXPR_EDS},
	{"vmd(k,1,10,1,k,rand,0)",EXPR_EVMD},
	{"med({1..1000000000000000})",EXPR_EMEM},
	{NULL}
};
void errcheck(const char *e,int expect){
	int error;
	printf("checking %s --- expect \"%s\"",e,expr_error(expect));
	if(new_expr6(e,"t",NULL,EXPR_IF_INSTANT_FREE,&error,NULL)){
		printf("\nerror! %s should be \"%s\" but ok\n",e,expr_error(expect));
		goto ab;
	}else if(error!=expect){
		printf("\nerror! %s should be \"%s\" but \"%s\"\n",e,expr_error(expect),expr_error(error));
		goto ab;
	}
	printf(" ... ok\n");
	return;
ab:
	printf("ABORTING\n");
	abort();
}
void check(const char *e,double expect){
	double r;
	//static int k=0;if(k++==39)exit(0);
	printf("checking %s --- expect %lg",e,expect);
	r=expr_calc5(e,"t",0,NULL,0);
	if(memcmp(&r,&expect,sizeof(double))){
		printf("\nerror! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	r=expr_calc5(e,"t",0,NULL,EXPR_IF_NOOPTIMIZE);
	if(memcmp(&r,&expect,sizeof(double))){
		printf("\noptimization error! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	printf(" ... ok\n");
	return;
ab:
	printf("ABORTING\n");
	abort();
}
int main(int argc,char **argv){
	srand48(time(NULL)+getpid());
	expr_calc5("t+2","t",3,NULL,0);
	for(const struct proj *p=projs;p->e;++p)
		check(p->e,p->expect);
	for(const struct eproj *p=eprojs;p->e;++p)
		errcheck(p->e,p->expect);
	new_expr7("t^3+sin(t)+sum(n,0,100,1,sin(n*t))","t",NULL,EXPR_IF_INSTANT_FREE,1250,NULL,NULL);
	return 0;
}
