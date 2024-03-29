/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <float.h>
#include <err.h>
#include "expr.h"
#define printval(x) fprintf(stderr,#x ":%lu\n",x)
#define printvall(x) fprintf(stderr,#x ":%ld\n",x)
#define printvald(x) fprintf(stderr,#x ":%lf\n",x)
#define NDEBUG
#include <assert.h>
//#define free(v)
static const char *eerror[]={"Unknown error","Unknown symbol","Parentheses do not match","Function and keyword must be followed by a \'(\'","No value in parenthesis","No enough or too much argument","Bad number","Target is not variable","Empty value","Unexpected operator","Zero-argument function must be followed by \'()\'","Defined symbol"};
const char *expr_error(int error){
	if(error<0)error=-error;
	if(error>=(sizeof(eerror)/sizeof(*eerror)))return eerror[0];
	else return eerror[error];
}
static const char spaces[]={" \t\r\f\n\v"};
static const char special[]={"+-*/%^(),<>=!&|"};
const static char ntoc[]={"0123456789abcdefg"};
uint64_t expr_gcd64(uint64_t x,uint64_t y){
	uint64_t r,r1;
	r=__builtin_ctzl(x);
	r1=__builtin_ctzl(y);
	r=r<r1?r:r1;
	x>>=r;
	y>>=r;
	r1=(x<y);
	while(x&&y){
		if(r1^=1)x%=y;
		else y%=x;
	}
	return (x|y)<<r;
}
void expr_memrand(void *restrict m,size_t n){
#if (RAND_MAX>=UINT32_MAX)
	while(n>=4){
		*(uint32_t *)m=(uint32_t)rand();
		*(char *)&m+=4;
		n-=4;
	}
#endif
#if (RAND_MAX>=UINT16_MAX)
	while(n>=2){
		*(uint16_t *)m=(uint16_t)rand();
		*(char *)&m+=2;
		n-=2;
	}
#endif
	while(n>0){
		*(uint8_t *)m=(uint8_t)rand();
		*(char *)&m+=1;
		--n;
	}

}
#define CALLOGIC(a,b,_s) ((fabs(a)>DBL_EPSILON) _s (fabs(b)>DBL_EPSILON))
#define CALBLOGIC(_sign_cal,_sign_zero,_zero_val) \
	uint64_t x2,x1;\
	int64_t expdiff=EXPR_EDEXP(&a)-EXPR_EDEXP(&b);\
	double swapbuf;\
	if(expdiff<0L){\
		swapbuf=a;\
		a=b;\
		b=swapbuf;\
		expdiff=-expdiff;\
	}\
	if(expdiff>52L)goto zero;\
	x2=(EXPR_EDBASE(&b)|(1UL<<52UL))>>expdiff;\
	x1=EXPR_EDBASE(&a)|(1UL<<52UL);\
	x1 _sign_cal x2;\
	if(x1){\
		x2=63UL-__builtin_clzl(x1);\
		x1&=~(1UL<<x2);\
		x2=52UL-x2;\
		if(EXPR_EDEXP(&a)<x2)goto zero;\
		EXPR_EDBASE(&a)=x1<<x2;\
		EXPR_EDEXP(&a)-=x2;\
	}else {\
		a=0.0;\
	}\
	EXPR_EDSIGN(&a) _sign_cal EXPR_EDSIGN(&b);\
	return a;\
zero:\
	return EXPR_EDSIGN(&a) _sign_zero EXPR_EDSIGN(&b)?-( _zero_val):(_zero_val)
double expr_and2(double a,double b){
	CALBLOGIC(&=,&&,0.0);
}
double expr_or2(double a,double b){
	CALBLOGIC(|=,||,a>=0.0?a:-a);
}
double expr_xor2(double a,double b){
	CALBLOGIC(^=,^,a>=0.0?a:-a);
}
double expr_gcd2(double x,double y){
	return (double)expr_gcd64((uint64_t)(fabs(x)+DBL_EPSILON),
		(uint64_t)(fabs(y)+DBL_EPSILON));
}
double expr_lcm2(double x,double y){
	uint64_t a=(uint64_t)(fabs(x)+DBL_EPSILON),b=(uint64_t)(fabs(y)+DBL_EPSILON);
	return (double)(a*b)/expr_gcd64(a,b);
}

static double expr_rand(size_t n,double *args){
	//assert(n==2);
	return args[0]+(args[1]-args[0])*drand48();
}
#define CALMDLOGIC(_symbol)\
	double ret=*(args++);\
	while(--n>0){\
		ret= _symbol (ret,*args);\
		++args;\
	}\
	return ret
static double expr_and(size_t n,double *args){
	CALMDLOGIC(expr_and2);
}
static double expr_or(size_t n,double *args){
	CALMDLOGIC(expr_or2);
}
static double expr_xor(size_t n,double *args){
	CALMDLOGIC(expr_xor2);
}
static double expr_gcd(size_t n,double *args){
	CALMDLOGIC(expr_gcd2);
}
static double expr_lcm(size_t n,double *args){
	CALMDLOGIC(expr_lcm2);
}
static double expr_sign(double x){
	if(x>DBL_EPSILON)return 1.0;
	else if(x<-DBL_EPSILON)return -1.0;
	else return 0.0;
}
#define expr_not(x) expr_xor2(x,9007199254740991.0/* 2^53-1*/)
static double expr_notfunc(double x){
	return expr_not(x);
}
static double expr_nnot(double x){
	if(fabs(x)>DBL_EPSILON)return 1.0;
	else return 0.0;
}
static double expr_fact(double x){
	double sum=1.0;
	x=floor(x);
	while(x>DBL_EPSILON){
		sum*=x;
		x-=1.0;
	}
	return sum;
}
static double expr_dfact(double x){
	double sum=1.0;
	x=floor(x);
	while(x>DBL_EPSILON){
		sum*=x;
		x-=2.0;
	}
	return sum;
}
static double expr_nfact(size_t n,double *args){
	double sum=1.0,x=args[0];
	x=floor(x);
	while(x>DBL_EPSILON){
		sum*=x;
		x-=args[1];
	}
	return sum;
}
static double expr_piece(size_t n,const struct expr *args,double input){
	const struct expr *arg0=args;
	--n;
	while(args-arg0<n){
		if(fabs(expr_eval(args++,input))>DBL_EPSILON){
			return expr_eval(args,input);
		}else {
			++args;
		}

	}
	return expr_eval(arg0+n,input);
}
static double expr_derivate(size_t n,const struct expr *args,double input){
	double epsilon=(n>=2?expr_eval(args+1,input):FLT_EPSILON);
	return (expr_eval(args,input+epsilon)-
		expr_eval(args,input-epsilon))/epsilon/2;
}
double expr_multilevel_derivate(const struct expr *ep,double input,long level,double epsilon){
	if(level<1l)
		return expr_eval(ep,input);
	else return (expr_multilevel_derivate(
		ep,input+epsilon,level-1,epsilon
		)-expr_multilevel_derivate(
			ep,input-epsilon,level-1,epsilon
			))/epsilon/2;
}
static double expr_multi_derivate(size_t n,const struct expr *args,double input){
	double epsilon=(n>=3?expr_eval(args+2,input):FLT_EPSILON);
	double level=(n>=2?expr_eval(args+1,input):1.0);
	return expr_multilevel_derivate(args,input,(long)(level+DBL_EPSILON),epsilon);
}
static double expr_root(size_t n,const struct expr *args,double input){
	//root(expression)
	//root(expression,from)
	//root(expression,from,to)
	//root(expression,from,to,step)
	//root(expression,from,to,step,epsilon)
	double epsilon=FLT_EPSILON,from=0.0,to=INFINITY,step=FLT_EPSILON,swapbuf;
	switch(n){
		case 5:
			epsilon=fabs(expr_eval(args+4,input));
		case 4:
			step=fabs(expr_eval(args+3,input));
		case 3:
			to=expr_eval(args+2,input);
		case 2:
			from=expr_eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	if(from>to){
		swapbuf=from;
		from=to;
		to=swapbuf;
	}
	for(;from<=to;from+=step){
		if(fabs(expr_eval(args,from))<=epsilon)
			return from;
	}
	return INFINITY;
}
static double expr_root2(size_t n,const struct expr *args,double input){
	//root2(expression)
	//root2(expression,from)
	//root2(expression,from,to)
	//root2(expression,from,to,step)
	//root2(expression,from,to,step,epsilon)
	double epsilon=0.0,from=0.0,to=INFINITY,step=1.0,swapbuf;
	int neg,trunc=0;
	switch(n){
		case 5:
			epsilon=fabs(expr_eval(args+4,input));
		case 4:
			step=fabs(expr_eval(args+3,input));
		case 3:
			to=expr_eval(args+2,input);
		case 2:
			from=expr_eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	if(from>to){
		swapbuf=from;
		from=to;
		to=swapbuf;
		trunc=1;
	}
	swapbuf=expr_eval(args,from);
	neg=(swapbuf<-0.0);
	if(fabs(swapbuf)<=epsilon)goto end;
	for(from+=step;from<=to;from+=step){
		if(from+step==from){
			//if(neg)from+=2.0*step;
			goto end1;
		}
		if(fabs(swapbuf=expr_eval(args,from))<=epsilon)goto end;
		//printf("neg=%d\tstep=%lg\t%.1024lf\n",neg,step,step);
		if((swapbuf<-0.0)==neg)continue;
		from-=step;
		if(step<=epsilon)goto end;
		do {
			step/=2.0;
		}while((expr_eval(args,from+step)<0.0)!=neg);
		from+=step;
	}
	return INFINITY;
end1:
	if(!trunc)swapbuf=expr_eval(args,from);
end:
	//from+=neg?2.0*step:step;
	if(!trunc&&fabs(swapbuf)>epsilon)
		return from+2.0*step;
	return from;
}
static double expr_rooti(size_t n,const struct expr *args,double input){
	//root2(expression)
	//root2(expression,from)
	//root2(expression,from,epsilon)
	double epsilon=DBL_EPSILON,from=1.0,swapbuf;
	switch(n){
		case 3:
			epsilon=fabs(expr_eval(args+4,input));
		case 2:
			from=expr_eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	for(;;){
		swapbuf=expr_multilevel_derivate(args,from,1,epsilon);
		if(fabs(swapbuf)==0.0){
			if(fabs(expr_eval(args,from))<=epsilon)
				return from;
			break;
		}
		swapbuf=from-expr_eval(args,from)/swapbuf;
		if(fabs(from-swapbuf)<=epsilon)return swapbuf;
		from=swapbuf;
	}
	return INFINITY;
}
static double expr_andl(size_t n,const struct expr *args,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(fabs(expr_eval(ep,input))<=DBL_EPSILON)
			return 0.0;
	}
	return 1.0;
}
static double expr_orl(size_t n,const struct expr *args,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(fabs(expr_eval(ep,input))>DBL_EPSILON)
			return 1.0;
	}
	return 0.0;
}
static double expr_max(size_t n,double *args){
	double ret=DBL_MIN;
	while(n>0){
		//printf("%lf\n",*args);
		if(*args>ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}
static double expr_min(size_t n,double *args){
	double ret=DBL_MAX;
	while(n>0){
		//printf("%lf\n",*args);
		if(*args<ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}

static double expr_hypot(size_t n,double *args){
	double ret=0;
	while(n>0){
		//printf("%lf\n",*args);
		ret+=*args**args;
		--n;
		++args;
	}
	return sqrt(ret);
}
//#define REGSYM(s) {#s,s}
#define REGFSYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.func=s},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGCSYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.value=s},.type=EXPR_CONSTANT}
#define REGFSYM2(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGMDSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION}
#define REGMDEPSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION}
#define REGCSYM2(s,val) {.strlen=sizeof(s)-1,.str=s,.un={.value=val},.type=EXPR_CONSTANT}
#define REGKEY(s,op,dim,desc) {s,op,dim,sizeof(s)-1,desc}
const struct expr_builtin_keyword expr_keywords[]={
	REGKEY("sum",EXPR_SUM,5,"index_name,start_index,end_index,index_step,addend"),
	REGKEY("int",EXPR_INT,5,"integral_var_name,upper_limit,lower_limit,epsilon,integrand"),
	REGKEY("prod",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"),
	REGKEY("pai",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"),
	REGKEY("sup",EXPR_SUP,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("infi",EXPR_INF,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("andn",EXPR_ANDN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("orn",EXPR_ORN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("xorn",EXPR_XORN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("gcdn",EXPR_GCDN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("lcmn",EXPR_LCMN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("for",EXPR_FOR,5,"var_name,start_var,cond,body,value"),
	REGKEY("loop",EXPR_LOOP,5,"var_name,start_var,count,body,value"),
	REGKEY("while",EXPR_WHILE,3,"cond,body,value"),
	REGKEY("if",EXPR_IF,3,"cond,if_value,else_value"),
	{NULL}
};
const struct expr_builtin_symbol expr_bsyms[]={
	REGFSYM2("abs",fabs),
	REGFSYM(acos),
	REGFSYM(acosh),
	REGFSYM(asin),
	REGFSYM(asinh),
	REGFSYM(atan),
	REGFSYM(atanh),
	REGFSYM(cbrt),
	REGFSYM(ceil),
	REGFSYM(cos),
	REGFSYM(cosh),
	REGFSYM2("dfact",expr_dfact),
	REGFSYM(erf),
	REGFSYM(exp),
	REGFSYM(exp2),
	REGFSYM(expm1),
	REGFSYM(fabs),
	REGFSYM2("fact",expr_fact),
	REGFSYM(floor),
	REGFSYM(j0),
	REGFSYM(j1),
	REGFSYM(lgamma),
	REGFSYM(log),
	REGFSYM2("ln",log),
	REGFSYM(log10),
	REGFSYM(log1p),
	REGFSYM(log2),
	REGFSYM(logb),
	REGFSYM(nearbyint),
	REGFSYM2("nnot",expr_nnot),
	REGFSYM2("not",expr_notfunc),
	REGFSYM(rint),
	REGFSYM(round),
	REGFSYM2("sign",expr_sign),
	REGFSYM(sin),
	REGFSYM(sinh),
	REGFSYM(sqrt),
	REGFSYM(tan),
	REGFSYM(tanh),
	REGFSYM(tgamma),
	REGFSYM(trunc),
	REGFSYM(y0),
	REGFSYM(y1),

	REGCSYM(DBL_MAX),
	REGCSYM(DBL_MIN),
	REGCSYM(DBL_EPSILON),
	REGCSYM(HUGE_VAL),
	REGCSYM(FLT_MAX),
	REGCSYM(FLT_MIN),
	REGCSYM(FLT_EPSILON),
	REGCSYM(HUGE_VALF),
	REGCSYM(INFINITY),
	REGCSYM2("inf",INFINITY),
	REGCSYM(NAN),
	REGCSYM2("nan",NAN),
	REGCSYM2("e",M_E),
	REGCSYM2("log2e",M_LOG2E),
	REGCSYM2("log10e",M_LOG10E),
	REGCSYM2("ln2",M_LN2),
	REGCSYM2("ln10",M_LN10),
	REGCSYM2("pi",M_PI),
	REGCSYM2("pi_2",M_PI_2),
	REGCSYM2("pi_4",M_PI_4),
	REGCSYM2("1_pi",M_1_PI),
	REGCSYM2("2_pi",M_2_PI),
	REGCSYM2("2_sqrtpi",M_2_SQRTPI),
	REGCSYM2("sqrt2",M_SQRT2),
	REGCSYM2("sqrt1_2",M_SQRT1_2),

	REGMDSYM2("and",expr_and,0),
	REGMDSYM2("or",expr_or,0),
	REGMDSYM2("xor",expr_xor,0),
	REGMDSYM2("gcd",expr_gcd,0),
	REGMDSYM2("hypot",expr_hypot,0),
	REGMDSYM2("lcm",expr_lcm,0),
	REGMDSYM2("min",expr_min,0),
	REGMDSYM2("max",expr_max,0),
	REGMDSYM2("nfact",expr_nfact,2ul),
	REGMDSYM2("rand",expr_rand,2ul),

	REGMDEPSYM2("andl",expr_andl,0),
	REGMDEPSYM2("orl",expr_orl,0),
	REGMDEPSYM2("piece",expr_piece,0),
	REGMDEPSYM2("d",expr_derivate,0),
	REGMDEPSYM2("dn",expr_multi_derivate,0),
	REGMDEPSYM2("root",expr_root,0),
	REGMDEPSYM2("root2",expr_root2,0),
	REGMDEPSYM2("rooti",expr_rooti,0),
	{.str=NULL}
};
const struct expr_builtin_symbol *expr_bsym_search(const char *sym,size_t sz){
	for(const struct expr_builtin_symbol *p=expr_bsyms;p->str;++p){
		//printf("sz=%zu,ps=%u %s\n",sz,p->strlen,p->str);
		if(sz==p->strlen&&!memcmp(p->str,sym,sz)){
			return p;
		}
	}
	return NULL;
}
const struct expr_builtin_symbol *expr_bsym_rsearch(void *addr){
	for(const struct expr_builtin_symbol *p=expr_bsyms;p->str;++p){
		if(p->un.uaddr==addr){
			return p;
		}
	}
	return NULL;
}
static void *xmalloc(size_t size){
	void *r=malloc(size);
	if(!r){
		warn("IN xmalloc(size=%zu)\n"
			"CANNOT ALLOCATE MEMORY",size);
		warnx("ABORTING");
		abort();
	}
	return r;
}
static void *xrealloc(void *old,size_t size){
	void *r=realloc(old,size);
	if(!r){
		warn("IN xrealloc(old=%p,size=%zu)\n"
			"CANNOT REALLOCATE MEMORY",old,size);
		warnx("ABORTING");
		abort();
	}
	return r;
}
static size_t expr_strcopy(const char *s,size_t sz,char *buf){
	const char *s0=s,*p,*s1;
	char *buf0=buf,v;
	while(s-s0<sz)switch(*s){
		case '\\':
			switch(s[1]){
				case '\\':
					*(buf++)='\\';
					s+=2;
					break;
				case 'a':
					*(buf++)='\a';
					s+=2;
					break;
				case 'b':
					*(buf++)='\b';
					s+=2;
					break;
				case 'c':
					*(buf++)='\0';
					s+=2;
					break;
				case 'e':
					*(buf++)='\033';
					s+=2;
					break;
				case 'f':
					*(buf++)='\f';
					s+=2;
					break;
				case 'n':
					*(buf++)='\n';
					s+=2;
					break;
				case 'r':
					*(buf++)='\r';
					s+=2;
					break;
				case 't':
					*(buf++)='\t';
					s+=2;
					break;
				case 'v':
					*(buf++)='\v';
					s+=2;
					break;
				case 'x':
					s1=s;
					p=s+=2;
					while(p-s0<sz&&((*p>='0'&&*p<='9')
						||(*p>='a'&&*p<='f')
						||(*p>='A'&&*p<='F')
						)&&p-s<2)++p;
					if(p==s)goto fail;
					v=0;
					while(s<p){
						v<<=4;
						switch(*s){
							case '0':
							case '1':
							case '2':
							case '3':
							case '4':
							case '5':
							case '6':
							case '7':
							case '8':
							case '9':
								v+=*s-'0';
								break;
							case 'a':
							case 'b':
							case 'c':
							case 'd':
							case 'e':
							case 'f':
								v+=*s-'a'+10;
								break;
							case 'A':
							case 'B':
							case 'C':
							case 'D':
							case 'E':
							case 'F':
								v+=*s-'A'+10;
								break;
							default:
								goto fail;
						}
						++s;
					}
					*(buf++)=v;
					break;
				default:
					s1=s;
					p=s+=1;
					while(p-s0<sz&&*p>='0'&&*p<='7'&&p-s<3)
						++p;;
					if(p==s)goto fail;
					v=0;
					while(s<p){
						v<<=3;
						v+=*s-'0';
						++s;
					}
					*(buf++)=v;
					break;
fail:
					*(buf++)=s1[1];
					s=s1+2;
					break;
			}
			break;
		default:
			*(buf++)=*(s++);
			break;
	}
	return buf-buf0;
}
size_t expr_strscan(const char *s,size_t sz,char *buf){
	char *buf0=buf;
	const char *p,*endp=s+sz;
	if(!sz||*s!='\"')return 0;
	for(;;){
	while(s<endp&&*s!='\"'&&!strchr(spaces,*s))++s;
	if(!(s<endp))return buf-buf0;
	//if(*s=='\"')
	++s;
	//else return buf-buf0;
	p=s;
	do {
		p=strchr(p+1,'\"');
	}while(p&&p>s&&p[-1]=='\\');
	if(!p||p<=s)return buf-buf0;
	buf+=expr_strcopy(s,p-s,buf);
	s=p+1;
	}
}
char *expr_astrscan(const char *s,size_t sz,size_t *outsz){
	char *buf;
	buf=xmalloc(sz);
	*outsz=expr_strscan(s,sz,buf);
	buf[*outsz]=0;
	if(!*buf){
		free(buf);
		return NULL;
	}
	return buf;
}
void expr_free(struct expr *restrict ep){
	struct expr_inst *ip;
	if(!ep)return;
	if(ep->data){
		ip=ep->data;
		for(size_t i=ep->size;i>0;--i){
			switch(ip->op){
				case EXPR_SUM:
				case EXPR_INT:
				case EXPR_PROD:
				case EXPR_SUP:
				case EXPR_INF:
				case EXPR_ANDN:
				case EXPR_ORN:
				case EXPR_XORN:
				case EXPR_GCDN:
				case EXPR_LCMN:
				case EXPR_FOR:
				case EXPR_LOOP:
					//expr_symset_free(ip->un.es->ep->sset);
					expr_free(ip->un.es->ep);
					expr_free(ip->un.es->from);
					expr_free(ip->un.es->to);
					expr_free(ip->un.es->step);
					free(ip->un.es);
					break;
				case EXPR_CALLMD:
					free(ip->un.em->args);
				case EXPR_CALLME:
					for(size_t i=0;i<ip->un.em->dim;++i)
						expr_free(ip->un.em->eps+i);
					free(ip->un.em->eps);
					free(ip->un.em);
					break;
				case EXPR_IF:
				case EXPR_WHILE:
					free(ip->un.eb->cond);
					free(ip->un.eb->body);
					free(ip->un.eb->value);
					free(ip->un.eb);
					break;
				case EXPR_CALLHOT:
					expr_free(ip->un.hotfunc);
					break;
				default:
					break;
			}
			++ip;
		}
		free(ep->data);
	}
	if(ep->vars){
		for(size_t i=0;i<ep->vsize;++i)
			free(ep->vars[i]);
		free(ep->vars);
	}
	if(ep->freeable)free(ep);
}
#define EXTEND_SIZE 1
#define EXTEND_DATA if(ep->size>=ep->length){\
	ep->data=xrealloc(ep->data,\
		(ep->length+=EXTEND_SIZE)*sizeof(struct expr_inst));\
	}
__attribute__((noinline))
struct expr_inst *expr_addop(struct expr *restrict ep,double *dst,void *src,enum expr_op op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.src=(double *)src;
	//printvald(*(double *)&src);
	return ip;
}

static double *expr_newvar(struct expr *restrict ep){
	double *r=xmalloc(sizeof(double));
	if(ep->vsize>=ep->vlength){
		ep->vars=xrealloc(ep->vars,
			(ep->vlength+=EXTEND_SIZE)*sizeof(double *));
	}
	*(ep->vars+ep->vsize++)=r;
	*r=NAN;
	return r;
}
static double *expr_createvar(struct expr *restrict ep,const char *symbol,size_t symlen){
	double *r=expr_newvar(ep);
	if(!ep->sset_shouldfree){
		if(!ep->sset)ep->sset=new_expr_symset();
		else ep->sset=expr_symset_clone(ep->sset);
		ep->sset_shouldfree=1;
	}
	expr_symset_addl(ep->sset,symbol,symlen,EXPR_VARIABLE,r);
	return r;
}
static const char *expr_findpair(const char *c){
	size_t lv=0;
	if(*c!='(')goto err;
	while(*c){
		switch(*c){
			case '(':
				++lv;
				break;
			case ')':
				--lv;
				if(!lv)return c;
				break;
			default:
				break;
		}
		++c;
	}
err:
	return NULL;
}
static const char *expr_unfindpair(const char *e,const char *c){
	size_t lv=0;
	if(*c!=')')goto err;
	while(c>=e){
		switch(*c){
			case ')':
				++lv;
				break;
			case '(':
				--lv;
				if(!lv)return c;
				break;
			default:
				break;
		}
		--c;
	}
err:
	return NULL;
}
static const char *expr_getsym(const char *c){
	while(*c&&!strchr(special,*c))
		++c;
	return c;
}
static const char *expr_getsym_expo(const char *c){
	const char *c0=c;
	while(*c&&!strchr(special,*c))
		++c;
	if(c-c0>=2&&(*c=='-'||*c=='+')&&(c[-1]=='e'||c[-1]=='E')&&((c[-2]<='9'&&c[-2]>=0)||c[-2]=='.')){
		return expr_getsym(c+1);
	}
	return c;
}
static int expr_atod2(const char *str,double *dst){
	int ret;
	char c;
	ret=sscanf(str,"%lf%c",dst,&c);
	return ret;
}
static int expr_atod(const char *str,size_t sz,double *dst){
	int ret;
	char *p=xmalloc(sz+1);
	p[sz]=0;
	memcpy(p,str,sz);
	ret=expr_atod2(p,dst);
	free(p);
	return ret;
}
static char *expr_tok(char *restrict str,char **restrict saveptr){
	if(str){
	//	puts(str);
		*saveptr=str;
	}else if(!**saveptr)return NULL;
	else {
		str=*saveptr;
	}
	while(**saveptr){
		if(**saveptr==','){
			**saveptr=0;
			++(*saveptr);
		//	puts(str);
			return str;
		}
		if(**saveptr=='('){
			*saveptr=(char *)expr_findpair(*saveptr);
			//puts(*saveptr);
			if(!*saveptr)return NULL;
		}
		++(*saveptr);
	}
	return str;
}
static char **expr_sep(struct expr *restrict ep,char *e){
	char *p,*p1,*p2,**p3=NULL,*p5;
	size_t len=0,s,sz;
	if(*e=='('){
		p1=(char *)expr_findpair(e);
		if(p1){
			if(!p1[1]){
				*p1=0;
				++e;
			}
		}else {
			ep->error=EXPR_EPT;
			return NULL;
		}
	}
	//puts(e);
	if(!*e){
		ep->error=EXPR_ENVP;
		return NULL;
	}
	for(p=expr_tok(e,&p2);p;p=expr_tok(NULL,&p2)){
		s=strlen(p);
		if((p5=expr_astrscan(p,s,&sz))){
			for(char *p4=p5;p4-p5<sz;++p4){
			p1=xmalloc(5);
			p1[0]='0';
			p1[1]='x';
			p1[2]=ntoc[*p4>>4];
			p1[3]=ntoc[*p4&15];
			p1[4]=0;
			p3=xrealloc(p3,(++len+1)*sizeof(char *));
			p3[len-1]=p1;
			}
			free(p5);
		}else {
			p1=xmalloc(s+1);
			p1[s]=0;
			memcpy(p1,p,s);
			p3=xrealloc(p3,(++len+1)*sizeof(char *));
			p3[len-1]=p1;
		}
	}
	if(p3)p3[len]=NULL;
	return p3;
}
static void expr_free2(char **buf){
	char **p=buf;
	while(*p){
		free(*p);
		++p;
	}
	free(buf);
}
static struct expr_mdinfo *expr_getmdinfo(struct expr *restrict ep,const char *e0,size_t sz,char *e,const char *asym,void *func,size_t dim,int ifep){
	char **v=expr_sep(ep,e);
	char **p;
	size_t i;
	struct expr_mdinfo *em;
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		//puts(*p);
		++p;
	}
	i=p-v;
	if(dim&&i!=dim){
		memcpy(ep->errinfo,e0,sz);
		ep->error=EXPR_ENEA;
		goto err1;
	}
	em=xmalloc(sizeof(struct expr_mdinfo));
	em->dim=i;

	em->un.func=func;
	em->eps=xmalloc(em->dim*sizeof(struct expr));
	em->args=NULL;
	if(!ifep)em->args=xmalloc(em->dim*sizeof(double));
	for(i=0;i<em->dim;++i){
		if(init_expr(em->eps+i,v[i],asym,ep->sset)<0){
			for(ssize_t k=i-1;k>=0;--k)
				expr_free(em->eps+k);
			goto err2;
		}
	}
	expr_free2(v);
	return em;
err2:
	if(em->args)free(em->args);
	ep->error=em->eps[i].error;
	memcpy(ep->errinfo,em->eps[i].errinfo,EXPR_SYMLEN);
	free(em->eps);
	free(em);
err1:
	expr_free2(v);
	return NULL;
}
static struct expr_suminfo *expr_getsuminfo(struct expr *restrict ep,const char *e0,size_t sz,char *e,const char *asym){
	char **v=expr_sep(ep,e);
	char **p;
	struct expr_suminfo *es;
	struct expr_symset *sset;
//	int error;
//	char ef[EXPR_SYMLEN];
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		//puts(*p);
		++p;
	}
		/*printf("is %zu\n",p-v);
		puts(v[0]);
		puts(v[1]);
		puts(v[2]);
		puts(v[3]);
		puts(v[4]);
		puts(v[5]);*/
	if(p-v!=5){
		memcpy(ep->errinfo,e0,sz);
		ep->error=EXPR_ENEA;
		goto err0;
	}
	es=xmalloc(sizeof(struct expr_suminfo));
//	sum(sym_index,from,to,step,expression)
	//sset=new_expr_symset();
	//puts("bef addv0");
	//expr_symset_add(sset,"draw",&es->index,EXPR_VARIABLE);
	sset=new_expr_symset();
	expr_symset_add(sset,v[0],EXPR_VARIABLE,&es->index);
	expr_symset_copy(sset,ep->sset);
	//printf("es->index %p\n",&es->index);
	es->from=new_expr(v[1],asym,ep->sset,&ep->error,ep->errinfo);
	if(!es->from)goto err1;
	es->to=new_expr(v[2],asym,sset,&ep->error,ep->errinfo);
	if(!es->to)goto err2;
	es->step=new_expr(v[3],asym,sset,&ep->error,ep->errinfo);
	if(!es->step)goto err3;
	//sset=expr_symset_clone(ep->sset);

	es->ep=new_expr(v[4],asym,sset,&ep->error,ep->errinfo);
	//printf("sset %p 1:%s 2:%s\n",es->ep->sset,es->ep->sset->syms[0].str,es->ep->sset->syms[1].str);
	if(!es->ep)goto err4;
	//printf("sset %p 1:%s 2:%s\n",sset,sset->syms[0].str,sset->syms[1].str);
	expr_free2(v);
	//assert(es->ep);
	//puts("getsi");
	expr_symset_free(sset);
	return es;
err4:
	//puts(v[0]);
	//printf("%s,%s\n",sset->syms[0].str,sset->syms[1].str);
	//assert(0);
	expr_free(es->step);
err3:
	expr_free(es->to);
err2:
	expr_free(es->from);
err1:
	free(es);
	expr_symset_free(sset);
err0:
	expr_free2(v);
	return NULL;
}
static struct expr_branchinfo *expr_getbranchinfo(struct expr *restrict ep,const char *e0,size_t sz,char *e,const char *asym){
	char **v=expr_sep(ep,e);
	char **p;
	struct expr_branchinfo *eb;
//	int error;
//	char ef[EXPR_SYMLEN];
	//assert(v);
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		//puts(*p);
		++p;
	}
	//assert(p-v==3);
	if(p-v!=3){
		memcpy(ep->errinfo,e0,sz);
		ep->error=EXPR_ENEA;
		goto err0;
	}
	eb=xmalloc(sizeof(struct expr_branchinfo));
//	while(cond,body,value)
	eb->cond=new_expr(v[0],asym,ep->sset,&ep->error,ep->errinfo);
	//assert(eb->cond);
	if(!eb->cond)goto err1;
	eb->body=new_expr(v[1],asym,ep->sset,&ep->error,ep->errinfo);
	//assert(eb->body);
	if(!eb->body)goto err2;
	eb->value=new_expr(v[2],asym,ep->sset,&ep->error,ep->errinfo);
	//assert(eb->value);
	if(!eb->value)goto err3;
	expr_free2(v);
	//assert(0);
	return eb;
err3:
	expr_free(eb->body);
err2:
	expr_free(eb->cond);
err1:
	free(eb);
//	ep->error=error;
//	memcpy(ep->errinfo,ef,EXPR_SYMLEN);
err0:
	expr_free2(v);
	return NULL;
}
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym,size_t asymlen);
static double *expr_getvalue(struct expr *restrict ep,const char *e,const char **_p,const char *asym,size_t asymlen){
	const char *p,*p2;//*e0=e
	char *buf;
	double *v0=NULL;
	int r0;
	union {
		double v;
		struct expr *ep;
		struct expr_suminfo *es;
		struct expr_branchinfo *eb;
		struct expr_mdinfo *em;
	} un;
	union {
		const struct expr_symbol *es;
		const struct expr_builtin_symbol *ebs;
	} sym;
	const union expr_symbol_value *sv;
	int type;
	size_t dim;
	//fprintf(stderr,"getval %u: %s\n",assign_level,e0);
	for(;;++e){
		switch(*e){
			case 0:
				ep->error=EXPR_EEV;
				return NULL;
	//	if(*e=='('){
			case '(':
				p=expr_findpair(e);
				if(!p){
pterr:
					ep->error=EXPR_EPT;
					//assert(0);
					return NULL;
				}
				p2=e;
				if(*(++p2)==')'){
					ep->error=EXPR_ENVP;
					//assert(0);
					return NULL;
				}
				buf=xmalloc(p-e);
				buf[p-e-1]=0;
				memcpy(buf,e+1,p-e-1);
				v0=expr_scan(ep,buf,asym,asymlen);
				free(buf);
				//assert(v0);
				if(!v0)return NULL;
				e=p+1;
				goto vend;
			case '+':
				continue;
			default:
				break;
		//}else if(*e=='+')continue;
		}
		//else if(*e==')'&&!expr_unfindpair(e0,e))goto pterr;
		p=expr_getsym(e);
		//fprintf(stderr,"unknown sym %ld %s\n",p-e,e);
		if(p==e){
			if(*e&&strchr(special,*e)){
				*ep->errinfo=*e;
				ep->error=EXPR_EUO;
				return NULL;
			}
			goto symerr;
		}
		for(const struct expr_builtin_keyword *kp=expr_keywords;
				kp->str;++kp){
			if(p-e!=kp->strlen||memcmp(e,kp->str,p-e))
				continue;
			if(*p!='('){
				memcpy(ep->errinfo,e,p-e);
				ep->error=EXPR_EFP;
				//assert(0);
				return NULL;
			}
			p2=e;
			e=p;
			p=expr_findpair(e);
			if(!p)goto pterr;
			buf=xmalloc(p-e+2);
			buf[p-e+1]=0;
			memcpy(buf,e,p-e+1);
			switch(kp->op){
				case EXPR_IF:
				case EXPR_WHILE:
					un.eb=expr_getbranchinfo(ep,p2,e-p2,buf,asym);
					break;
				default:
					un.es=expr_getsuminfo(ep,p2,e-p2,buf,asym);
					break;
			}
			free(buf);
			//assert(es);
			if(!un.es){
				/*if(ep->error==EXPR_ENEA)
					memcpy(ep->errinfo,p2,e-p2);*/
				return NULL;
			}
			v0=expr_newvar(ep);
			expr_addop(ep,v0,un.es,kp->op);
			e=p+1;
			goto vend;
		}
		if(asym&&p-e==asymlen&&!memcmp(e,asym,p-e)){
			v0=expr_newvar(ep);
			expr_addinput(ep,v0);
			//fprintf(stderr,"asym %ld %s\n",p-e,e);
			e=p;
			goto vend;
		}
		//p1=expr_sym2addr(ep,e,p-e,&type,&dim);
		//fprintf(stderr,"find sym %ld %s\n",p-e,e);
		if(ep->sset&&(sym.es=expr_symset_search(ep->sset,e,p-e))){
			type=sym.es->type;
			switch(type){
				case EXPR_MDFUNCTION:
				case EXPR_MDEPFUNCTION:
					dim=*(size_t *)(sym.es->str+sym.es->strlen+1);
				default:
					break;
			}
			sv=&sym.es->un;
		}else if((sym.ebs=expr_bsym_search(e,p-e))){
			type=sym.ebs->type;
			sv=&sym.ebs->un;
			dim=sym.ebs->dim;
		}else goto number;
		switch(type){
			case EXPR_FUNCTION:
			//if(type==EXPR_FUNCTION){
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getvalue(ep,p,&e,asym,asymlen);
				//assert(v0);
				if(!v0)return NULL;
				expr_addcall(ep,v0,sv->func);
				break;
			case EXPR_ZAFUNCTION:
			//}else if(type==EXPR_ZAFUNCTION){
				if(*p!='('||p[1]!=')'){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EZAFP;
					//assert(0);
					return NULL;
				}
				v0=expr_newvar(ep);
				//assert(v0);
				//if(!v0)return NULL;
				expr_addcallza(ep,v0,sv->zafunc);
				e=p+2;
				break;
			case EXPR_HOTFUNCTION:
			//}else if(type==EXPR_HOTFUNCTION){
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getvalue(ep,p,&e,asym,asymlen);
				//assert(v0);
				if(!v0)return NULL;
				un.ep=new_expr(sv->hotexpr,sv->hotexpr+
					strlen(sv->hotexpr)+1
					,ep->sset,&ep->error,ep->errinfo);
				if(!un.ep)return NULL;
				expr_addcallhot(ep,v0,un.ep);
				break;
			//}else if(type==EXPR_CONSTANT){
			case EXPR_CONSTANT:
				v0=expr_newvar(ep);
				expr_addconst(ep,v0,sv->value);
				//printf("%p %lf\n",p1,*(double *)p1);
				//v0=p1;
				e=p;
				break;
			case EXPR_VARIABLE:
			//}else if(type==EXPR_VARIABLE){
				v0=expr_newvar(ep);
				expr_addcopy(ep,v0,(void *)sv->addr);
				//v0=p1;
				/*if(assign_level){
					//struct expr_inst *ip=
					expr_addcopy(ep,(void *)sv->addr,NULL)
					->assign_level=1;
					assign_level=0;
				}*/
				e=p;
				break;
			case EXPR_MDFUNCTION:
			case EXPR_MDEPFUNCTION:
			//}else if(type==EXPR_MDFUNCTION
			//	||type==EXPR_MDEPFUNCTION){
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				p2=e;
				e=p;
				p=expr_findpair(e);
				if(!p)goto pterr;
				buf=xmalloc(p-e+2);
				buf[p-e+1]=0;
				memcpy(buf,e,p-e+1);
				if(type==EXPR_MDEPFUNCTION)
					un.em=expr_getmdinfo(ep,p2,e-p2,buf,asym,
					sv->mdepfunc,dim,1);
				else	
					un.em=expr_getmdinfo(ep,p2,e-p2,buf,asym,
					sv->mdfunc,dim,0);
				free(buf);
				//assert(es);
				if(!un.em){
					/*if(ep->error==EXPR_ENEA)
					memcpy(ep->errinfo,p2,e-p2);*/
					return NULL;
				}
				v0=expr_newvar(ep);
				switch(type){
					case EXPR_MDFUNCTION:
						expr_addcallmd(ep,v0,un.em);
						break;
					case EXPR_MDEPFUNCTION:
						expr_addcallmdep(ep,v0,un.em);
						break;
				}
				e=p+1;
				break;
			//}else 
			default:
				goto symerr;
			break;
		}
		goto vend;
number:
		p=expr_getsym_expo(e);
		r0=expr_atod(e,p-e,&un.v);
		if(r0==1){
			//v1=expr_newvar(ep);
			v0=expr_newvar(ep);
			expr_addconst(ep,v0,un.v);
			//*v1=un.v;
			e=p;
			goto vend;
		}else if(r0>1){
			memcpy(ep->errinfo,e,p-e);
			ep->error=EXPR_ENUMBER;
			return NULL;
		}
symerr:
		memcpy(ep->errinfo,e,p-e);
		ep->error=EXPR_ESYMBOL;
		return NULL;
	}
vend:
	if(_p)*_p=e;
	/*if(assign_level){
		ep->error=EXPR_ETNV;
		return NULL;
	}*/
	return v0;
}
struct expr_vnode {
	struct expr_vnode *next;
	double *v;
	enum expr_op op;
	unsigned int unary;
};
static struct expr_vnode *expr_vn(double *v,enum expr_op op,unsigned int unary){
	struct expr_vnode *p;
	p=xmalloc(sizeof(struct expr_vnode));
	p->next=NULL;
	p->v=v;
	p->op=op;
	p->unary=unary;
	return p;
}
static struct expr_vnode *expr_vnadd(struct expr_vnode *vp,double *v,enum expr_op op,unsigned int unary){
	struct expr_vnode *p;
	//fprintf(stderr,"vp %p\n",vp);
	if(!vp)return expr_vn(v,op,unary);
	for(p=vp;p->next;p=p->next);
//	//fprintf(stderr,"p %p\n",p);
//	//fprintf(stderr,"p %p\n",p);
	p->next=xmalloc(sizeof(struct expr_vnode));
//	//fprintf(stderr,"p->next %p\n",p->next);
	p->next->v=v;
	p->next->op=op;
	p->next->unary=unary;
	p->next->next=NULL;
	return vp;
}

static int expr_do_unary(struct expr *restrict ep,struct expr_vnode *ev,int prec){
	int r=0;;
	for(struct expr_vnode *p=ev;p;p=p->next){
redo:
	switch(prec){
		case 2:
			//printf("is %d\n",p->unary);
			switch(p->unary&3){
				case 2:
					expr_addnotl(ep,p->v);
					break;
				case 3:
					expr_addnot(ep,p->v);
					break;
				default:
					continue;
			}
			r=1;
			p->unary>>=2;
			//if(!p->unary)continue;
			goto redo;
		case 4:
			//printf("is %d\n",p->unary);
			switch(p->unary&3){
				case 1:
					expr_addneg(ep,p->v);
					break;
				default:
					continue;
			}
			r=1;
			p->unary>>=2;
			//if(!p->unary)continue;
			goto redo;
		default:
			continue;
	}

	}
	return r;
}
static void expr_vnunion(struct expr *restrict ep,struct expr_vnode *ev){
	struct expr_vnode *p;

	expr_addop(ep,ev->v,ev->next->v,ev->next->op);
	/*if(ev->next->op==EXPR_ASSIGN){
		for(size_t i=0;i<ep->size;++i){
			if(ep->data[i].op==EXPR_COPY&&ep->data[i].assign_level){
				//printval((uint64_t)ep->data[i].un.src);
				//puts("");
				//printval((uint64_t)e);
				ep->data[i].un.src=ev->v;
				ep->data[i].assign_level=0;
				break;
			}
		}
	}*/
	p=ev->next;
	ev->next=ev->next->next;

	free(p);
}
static void expr_vnfree(struct expr_vnode *vp){
	struct expr_vnode *p;
	while(vp){
		p=vp->next;
		free(vp);
		vp=p;
	}
}
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym,size_t asymlen){
	double *v1;
	const char *e0=e;
	enum expr_op op=0;
	unsigned int unary;
	struct expr_vnode *ev=NULL,*p;
	//fprintf(stderr,"scan %s\n",e);

	do {
	unary=0;
redo:
	switch(*e){
		case '-':
		case '!':
		case '~':
			if(unary&0xc0000000){
				if(*e&&strchr(special,*e)){
					*ep->errinfo=*e;
					ep->error=EXPR_EUO;
				}else {
					ep->error=EXPR_EEV;
				}
				goto err;
			}
		default:
			break;
	}
	switch(*e){
		case '-':
			unary=(unary<<2)|1;
			++e;
			goto redo;
		case '!':
			unary=(unary<<2)|2;
			++e;
			goto redo;
		case '~':
			unary=(unary<<2)|3;
			++e;
			goto redo;
	}
//	v0=expr_getvalue(ep,e,&e,asym,0);
//	if(!v0)goto err;
//	ev=expr_vn(v0,0);
	v1=NULL;
		//printvall((long)op);
		v1=expr_getvalue(ep,e,&e,asym,asymlen);
		//assert(!ep->error);
		if(!v1)goto err;
		ev=expr_vnadd(ev,v1,op,unary);
	op=EXPR_END;
	for(;*e;){
		switch(*e){
			case '>':
				if(e[1]=='='){
					op=EXPR_GE;
					++e;
				}else if(e[1]=='>'){
					op=EXPR_SHR;
					++e;
				}else if(e[1]=='<'){
					op=EXPR_NE;
					++e;
				}else op=EXPR_GT;
				++e;
				goto end1;
			case '<':
				if(e[1]=='='){
					op=EXPR_LE;
					++e;
				}else if(e[1]=='<'){
					op=EXPR_SHL;
					++e;
				}else if(e[1]=='>'){
					op=EXPR_NE;
					++e;
				}else op=EXPR_LT;
				++e;
				goto end1;
			case '=':
				if(e[1]=='='){
					op=EXPR_EQ;
					++e;
				}else op=EXPR_SEQ;
				++e;
				goto end1;
			case '!':
				if(e[1]=='='){
					++e;
					op=EXPR_NE;
				}
				++e;
				goto end1;
			case '&':
				if(e[1]=='&'){
					op=EXPR_ANDL;
					++e;
				}
				else op=EXPR_AND;
				++e;
				goto end1;
			case '|':
				if(e[1]=='|'){
					op=EXPR_ORL;
					++e;
				}
				else op=EXPR_OR;
				++e;
				goto end1;
			case '+':
				op=EXPR_ADD;
				++e;
				goto end1;
			case '-':
				if(e[1]=='>'){
					const struct expr_symbol *esp=NULL;
					const char *p1=expr_getsym(e+=2);
					if(p1==e){
						if(*e&&strchr(special,*e)){
							*ep->errinfo=*e;
							ep->error=EXPR_EUO;
						}else {
							ep->error=EXPR_EEV;
						}
					goto err;
					}
					if(p1-e==asymlen&&!memcmp(e,asym,p1-e))
						goto tnv;
					if(ep->sset)
					esp=expr_symset_search(ep->sset,e,p1-e);
					if(!esp){
						if(expr_bsym_search(e,p1-e))
							goto tnv;
						ep->error=EXPR_ESYMBOL;
						memcpy(ep->errinfo,e,p1-e);
						goto err;
					}
					if(esp->type!=EXPR_VARIABLE){
tnv:
						ep->error=EXPR_ETNV;
						memcpy(ep->errinfo,e,p1-e);
						goto err;
					}
					expr_addcopy(ep,esp->un.addr,v1);
					e=p1;
					continue;
				}else if(e[1]&&e[1]=='-'&&e[2]=='>'){
					const char *p1=expr_getsym(e+=3);
					double *v2;
					if(p1==e){
						if(*e&&strchr(special,*e)){
							*ep->errinfo=*e;
							ep->error=EXPR_EUO;
						}else {
							ep->error=EXPR_EEV;
						}
					goto err;
					}
					if((p1-e==asymlen&&!memcmp(e,asym,p1-e))
					||(expr_bsym_search(e,p1-e)
					||(ep->sset&&expr_symset_search(ep->sset,e,p1-e)))){
						ep->error=EXPR_EDS;
						memcpy(ep->errinfo,e,p1-e);
						goto err;
					}
					v2=expr_createvar(ep,e,p1-e);
					expr_addcopy(ep,v2,v1);
					e=p1;
					continue;
				}else {
					op=EXPR_SUB;
					++e;
				}
				goto end1;
			case '*':
				op=EXPR_MUL;
				++e;
				goto end1;
			case '/':
				op=EXPR_DIV;
				++e;
				goto end1;
			case '%':
				op=EXPR_MOD;
				++e;
				goto end1;
			case '^':
				if(e[1]=='^'){
					if(e[2]=='^'){
						op=EXPR_XORL;
						++e;
					}else {
						op=EXPR_XOR;
					}
					++e;
				}else op=EXPR_POW;
				++e;
				goto end1;
			case ',':
				op=EXPR_COPY;
				++e;
				goto end1;
			case ')':
				if(!expr_unfindpair(e0,e)){
					ep->error=EXPR_EPT;
					goto err;
				}
			default:
				goto end1;
		}
	}
end1:
		continue;	
	}while(op!=EXPR_END);
#define SETPREC1(a)\
	for(p=ev;p;p=p->next){\
		while(p->next&&p->next->op==(a)){\
			expr_vnunion(ep,p);\
		}\
	}
#define SETPREC2(a,b)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			)){\
			expr_vnunion(ep,p);\
		}\
	}
#define SETPREC3(a,b,c)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			)){\
			expr_vnunion(ep,p);\
		}\
	}
#define SETPREC4(a,b,c,d)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			||p->next->op==(d)\
			)){\
			expr_vnunion(ep,p);\
		}\
	}
	//SETPREC1(EXPR_ASSIGN)
	//if(notl)expr_addnotl(ep,ev->v);
	//if(not)expr_addnot(ep,ev->v);
	expr_do_unary(ep,ev,2);
	SETPREC1(EXPR_POW)
	expr_do_unary(ep,ev,4);
	while(expr_do_unary(ep,ev,2)&&expr_do_unary(ep,ev,4));
	SETPREC3(EXPR_MUL,EXPR_DIV,EXPR_MOD)
	//if(neg)expr_addneg(ep,ev->v);
	SETPREC2(EXPR_ADD,EXPR_SUB)
	SETPREC2(EXPR_SHL,EXPR_SHR)
	SETPREC4(EXPR_LT,EXPR_LE,EXPR_GT,EXPR_GE)
	SETPREC3(EXPR_SEQ,EXPR_EQ,EXPR_NE)
	SETPREC1(EXPR_AND)
	SETPREC1(EXPR_XOR)
	SETPREC1(EXPR_OR)
	SETPREC1(EXPR_ANDL)
	SETPREC1(EXPR_XORL)
	SETPREC1(EXPR_ORL)
	while(ev->next)
		expr_vnunion(ep,ev);
	v1=ev->v;
	expr_vnfree(ev);
	return v1;
err:
	if(ev)expr_vnfree(ev);
	return NULL;
}
void init_expr_symset(struct expr_symset *restrict esp){
	/*esp->syms=NULL;
	esp->size=0;
	esp->depth=0;
	esp->length=0;
	esp->freeable=0;*/
	memset(esp,0,sizeof(struct expr_symset));
}
struct expr_symset *new_expr_symset(void){
	struct expr_symset *ep=xmalloc(sizeof(struct expr_symset));
	init_expr_symset(ep);
	ep->freeable=1;
	return ep;
}

static void expr_symbol_free(struct expr_symbol *restrict esp){
	for(int i=0;i<EXPR_SYMNEXT;++i){
		if(!esp->next[i])continue;
		expr_symbol_free(esp->next[i]);
	}
	free(esp);
}
void expr_symset_free(struct expr_symset *restrict esp){
	if(esp->syms)
		expr_symbol_free(esp->syms);
	if(esp->freeable)free(esp);
}
static int expr_firstdiff(const char *restrict s1,const char *restrict s2,size_t len){
	int r;
	do {
		r=(unsigned int)*(s1++)-(unsigned int)*(s2++);
		if(r)break;
	}while(--len);
	return r;
}
static int expr_strdiff(const char *restrict s1,size_t len1,const char *restrict s2,size_t len2,int *sum){
	int r;
	if(len1==len2){
		r=memcmp(s1,s2,len1);
		if(!r)return 0;
		*sum=r;
		return 1;
	}
	*sum=expr_firstdiff(s1,s2,len1<len2?len1:len2);
	return 1;
}
#define modi(d,m){\
	int tmpvar;\
	tmpvar=(d)%(m);\
	if((d)>=0||!tmpvar)\
		(d)=tmpvar;\
	else\
		(d)=((d)+((-(d))/(m)+!!tmpvar)*(m))%(m);\
}
static struct expr_symbol **expr_symset_findtail(struct expr_symset *restrict esp,const char *sym,size_t symlen,size_t *depth){
	struct expr_symbol *p;
	size_t dep;
	int r;
	if(!esp->syms){
		*depth=1;
		return &esp->syms;
	}
	dep=2;
	for(p=esp->syms;;++dep){
		if(!expr_strdiff(sym,symlen,p->str,p->strlen,&r))return NULL;
		modi(r,EXPR_SYMNEXT)
		//printf("diff:%d\n",r);
		if(p->next[r]){
			p=p->next[r];
		}else {
			*depth=dep;
			return p->next+r;
		}
	}
}
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,type);
	r=expr_symset_vadd(esp,sym,type,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symset_addl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,type);
	r=expr_symset_vaddl(esp,sym,symlen,type,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,va_list ap){
	return expr_symset_vaddl(esp,sym,strlen(sym),type,ap);
}
struct expr_symbol *expr_symset_vaddl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,va_list ap){
	struct expr_symbol *ep,**next;
	size_t len,len_expr,len_asym,depth;
	char *asymp;
	const char *p,*p1;
	if(!symlen)return NULL;
	if(symlen>=EXPR_SYMLEN)symlen=EXPR_SYMLEN-1;
	next=expr_symset_findtail(esp,sym,symlen,&depth);
	if(!next)return NULL;
	len=sizeof(struct expr_symbol)+symlen+1;
	ep=xmalloc(len);
	memset(&ep->next,0,sizeof(ep->next));
	ep->length=len;
	switch(type){
		case EXPR_CONSTANT:
			ep->un.value=va_arg(ap,double);
			break;
		case EXPR_VARIABLE:
			ep->un.addr=va_arg(ap,double *);
			break;
		case EXPR_FUNCTION:
			ep->un.func=va_arg(ap,double (*)(double));
			break;
		case EXPR_MDFUNCTION:
			ep->un.mdfunc=va_arg(ap,double (*)(size_t,double *));
			ep->length+=sizeof(size_t);
			ep=xrealloc(ep,ep->length);
			*(size_t *)(ep->str+ep->strlen+1)
			=va_arg(ap,size_t);
			break;
		case EXPR_MDEPFUNCTION:
			ep->un.mdepfunc=va_arg(ap,double (*)(size_t,
				const struct expr *,double));
			ep->length+=sizeof(size_t);
			ep=xrealloc(ep,ep->length);
			*(size_t *)(ep->str+ep->strlen+1)
			=va_arg(ap,size_t);
			break;
		case EXPR_HOTFUNCTION:
			p=va_arg(ap,const char *);
			p1=va_arg(ap,const char *);
			len_expr=strlen(p);
			len_asym=strlen(p1)+2;
			ep->length+=len_expr+len_asym;
			ep=xrealloc(ep,ep->length);
			ep->un.hotexpr=ep->str+symlen+1;
			asymp=ep->un.hotexpr+len_expr+1;
			memcpy(ep->un.hotexpr,p,len_expr);
			ep->un.hotexpr[len_expr]=0;
			memcpy(asymp,p1,len_asym);
			asymp[len_asym]=0;
			break;
		case EXPR_ZAFUNCTION:
			ep->un.zafunc=va_arg(ap,double (*)(void));
			break;
		default:
			free(ep);
			return NULL;
	}
	memcpy(ep->str,sym,symlen);
	ep->str[symlen]=0;
	ep->strlen=symlen;
	ep->type=type;
	ep->flag=0;
	++esp->size;
	esp->length+=ep->length;
	if(depth>esp->depth)esp->depth=depth;
	*next=ep;
	return ep;
}
struct expr_symbol *expr_symset_addcopy(struct expr_symset *restrict esp,const struct expr_symbol *restrict es){
	size_t depth;
	struct expr_symbol *restrict *ep=expr_symset_findtail(esp,es->str,es->strlen,&depth);
	if(ep){
		*ep=xmalloc(es->length);
	}else {
		return NULL;
	}
	memcpy(*ep,es,es->length);
	memset(&(*ep)->next,0,sizeof((*ep)->next));
	++esp->size;
	esp->length+=es->length;
	if(depth>esp->depth)esp->depth=depth;
	return *ep;
}
struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz){
	int r;
	for(struct expr_symbol *p=esp->syms;p;){
		if(!expr_strdiff(sym,sz,p->str,p->strlen,&r)){
			return p;
		}
		modi(r,EXPR_SYMNEXT)
		p=p->next[r];
	}
	return NULL;
}
static struct expr_symbol *expr_symset_rsearch_symbol(struct expr_symbol *esp,void *addr){
	struct expr_symbol *p;
	if(esp->un.uaddr==addr)return esp;
	for(int i=0;i<EXPR_SYMNEXT;++i){
		if(!esp->next[i])continue;
		p=expr_symset_rsearch_symbol(esp->next[i],addr);
		if(p)return p;
	}
	return NULL;
}
struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr){
	if(!esp->syms)return NULL;
	return expr_symset_rsearch_symbol(esp->syms,addr);
}
static void expr_symset_copy_symbol(struct expr_symset *restrict dst,const struct expr_symbol *restrict src){
	expr_symset_addcopy(dst,src);
	for(int i=0;i<EXPR_SYMNEXT;++i){
		if(!src->next[i])continue;
		expr_symset_copy_symbol(dst,src->next[i]);
	}
}
void expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src){
	if(src&&src->syms)
		expr_symset_copy_symbol(dst,src->syms);
}
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict ep){
	struct expr_symset *es=new_expr_symset();
	expr_symset_copy(es,ep);
	return es;
}
static void expr_strcpy_nospace(char *restrict s1,const char *restrict s2){
	const char *s20=s2;
	int instr=0;
	for(;*s2;++s2){
		if(*s2=='\"'&&s2>s20&&s2[-1]!='\\')instr^=1;
		if(!instr&&strchr(spaces,*s2))continue;
		*(s1++)=*s2;
	}
	*s1=0;
}
int init_expr_old(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	double *p;
	char *ebuf;
	/*ep->data=NULL;
	ep->vars=NULL;
	ep->sset_shouldfree=0;
	ep->error=0;
	ep->freeable=0;
	memset(ep->errinfo,0,EXPR_SYMLEN);
	ep->length=ep->size=ep->vlength=ep->vsize=0;*/
	memset(ep,0,sizeof(struct expr));
	ep->sset=esp;
	
	if(e){
		ebuf=xmalloc(strlen(e)+1);
		expr_strcpy_nospace(ebuf,e);
		p=expr_scan(ep,ebuf,asym,strlen(asym));
		free(ebuf);
		if(ep->sset_shouldfree){
			free(ep->sset);
			ep->sset=esp;
		}
		if(p){
			expr_addend(ep,p);
		}else {
			return -1;
		}
	}
	return 0;
}
static void expr_optimize(struct expr *restrict ep);
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	if(!init_expr_old(ep,e,asym,esp))
		expr_optimize(ep);
	else return -1;
	return 0;
}
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]){

	struct expr *ep=xmalloc(sizeof(struct expr));
	if(init_expr(ep,e,asym,esp)){
		*error=ep->error;
		if(errinfo)memcpy(errinfo,ep->errinfo,EXPR_SYMLEN);
		free(ep);
		return NULL;
	}
	ep->freeable=1;
	return ep;
}
static int expr_varofep(const struct expr *restrict ep,double *v){
	for(size_t i=0;i<ep->vsize;++i){
		if(ep->vars[i]==v){
	//		printf("%p is var %zu\n",v,i);
			return 1;
		}
	}
		//puts("var no at");
	return 0;
}
static void expr_writeconsts(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&
			expr_varofep(ep,ip->dst)
			){
			*ip->dst=ip->un.value;
		}
	}
}
static void expr_optimize_completed(struct expr *restrict ep){
	struct expr_inst *cip=ep->data;
	for(struct expr_inst *ip=cip;ip-ep->data<ep->size;++ip){
		if(ip->dst){
			if(ip>cip)
				memcpy(cip,ip,sizeof(struct expr_inst));
			++cip;
		}
	}
	ep->size=cip-ep->data;
	expr_writeconsts(ep);
}
static int expr_modified(const struct expr *restrict ep,double *v){
	if(!expr_varofep(ep,v))
		return 1;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->dst==v&&ip->op!=EXPR_CONST){
			return 1;
		}
	}
	return 0;
}
static int expr_usesrc(enum expr_op op);

static struct expr_inst *expr_findconst(const struct expr *restrict ep,struct expr_inst *ip){
	double *s=ip->dst;
	for(--ip;ip>=ep->data;--ip){
		if(expr_usesrc(ip->op)&&ip->un.src==s)break;
		if(ip->dst!=s)continue;
		if(ip->op==EXPR_CONST)return ip;
		else break;
	}
	return NULL;
}
static void expr_optimize_contadd(struct expr *restrict ep){
	double sum;
	double *nv;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst
			||!expr_modified(ep,ip->dst)
			||(ip->op!=EXPR_ADD&&ip->op!=EXPR_SUB)
			||expr_modified(ep,ip->un.src)
			)continue;
	//	abort();
		sum=ip->op==EXPR_ADD?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_ADD
				&&ip1->op!=EXPR_SUB
				)break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_ADD)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst=NULL;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			rip->un.value+=sum;
			ip->dst=NULL;
		}else {
			nv=expr_newvar(ep);
			ip->un.src=nv;
			if(sum<0.0){
				*nv=-sum;
				ip->op=EXPR_SUB;
			}
			else {
				*nv=sum;
				ip->op=EXPR_ADD;
			}
		}
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contsh(struct expr *restrict ep){
	double sum;
	double *nv;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst
			||!expr_modified(ep,ip->dst)
			||(ip->op!=EXPR_SHL&&ip->op!=EXPR_SHR)
			||expr_modified(ep,ip->un.src)
			)continue;
		sum=ip->op==EXPR_SHL?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_SHL
				&&ip1->op!=EXPR_SHR
				)break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_SHL)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst=NULL;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			EXPR_EDEXP(&rip->un.value)+=(int64_t)sum;
			ip->dst=NULL;
		}else {
			nv=expr_newvar(ep);
			ip->un.src=nv;
			if(sum<0.0){
				*nv=-sum;
				ip->op=EXPR_SHR;
			}
			else {
				*nv=sum;
				ip->op=EXPR_SHL;
			}
		}
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contmul(struct expr *restrict ep,enum expr_op op){
	double sum;
	double *nv;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst
			||!expr_modified(ep,ip->dst)
			||ip->op!=op
			||expr_modified(ep,ip->un.src)
			)continue;
		rip=expr_findconst(ep,ip);
		if(rip){
			sum=rip->un.value;
		}else {
			switch(op){
				case EXPR_POW:
				case EXPR_MOD:
				case EXPR_LT:
				case EXPR_LE:
				case EXPR_GT:
				case EXPR_GE:
				case EXPR_EQ:
				case EXPR_SEQ:
				case EXPR_NE:
					continue;
				default:
					break;
			}
			sum=*ip->un.src;
		}
		for(struct expr_inst *ip1=ip+!rip;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=op)break;
			if(!expr_modified(ep,(double *)ip1->un.src)){
			switch(op){
				case EXPR_ADD:
					sum+=*ip1->un.src;
					break;
				case EXPR_SUB:
					if(rip)sum-=*ip1->un.src;
					else sum+=*ip1->un.src;
					break;
				case EXPR_MUL:
					sum*=*ip1->un.src;
					break;
				case EXPR_DIV:
					if(rip)sum/=*ip1->un.src;
					else sum*=*ip1->un.src;
					break;
				case EXPR_MOD:
					sum=fmod(sum,*ip1->un.src);
					break;
				case EXPR_AND:
					sum=expr_and2(sum,*ip1->un.src);
					break;
				case EXPR_OR:
					sum=expr_or2(sum,*ip1->un.src);
					break;
				case EXPR_XOR:
					sum=expr_xor2(sum,*ip1->un.src);
					break;
				case EXPR_POW:
					sum=pow(sum,*ip1->un.src);
					break;
				case EXPR_COPY:
					sum=*ip1->un.src;
					break;
				case EXPR_GT:
					sum=sum>*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_LT:
					sum=sum<*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_GE:
					sum=sum>=*ip->un.src-DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_LE:
					sum=sum<=*ip->un.src+DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_SEQ:
					sum=sum==*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_EQ:
					sum=fabs(sum-*ip->un.src)<=DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_NE:
					sum=fabs(sum-*ip->un.src)>DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_ANDL:
					sum=CALLOGIC(sum,*ip->un.src,&&)?
						1.0:
						0.0;
					break;
				case EXPR_ORL:
					sum=CALLOGIC(sum,*ip->un.src,||)?
						1.0:
						0.0;
					break;
				case EXPR_XORL:
					sum=CALLOGIC(sum,*ip->un.src,^)?
						1.0:
						0.0;
					break;
				default:
					abort();
			}
				ip1->dst=NULL;
			}
		}
		if(rip){
			rip->un.value=sum;
			ip->dst=NULL;
		}else {
			nv=expr_newvar(ep);
			*nv=sum;
			ip->un.src=nv;
		}
	}
	expr_optimize_completed(ep);
}
static double expr_zero_element(enum expr_op op){
	switch(op){
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_OR:
		case EXPR_XOR:
		case EXPR_SHL:
		case EXPR_SHR:
			return 0.0;
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
		case EXPR_POW:
			return 1.0;
		default:
			return -1.0;
	}
}
static int expr_optimize_zero(struct expr *restrict ep){
	double ze;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			/*case EXPR_GT:
				if(*ip->un.src!=-INFINITY)break;
			case EXPR_GE:
				if(*ip->un.src!=-INFINITY&&
				*ip->un.src!=DBL_MIN)break;
			case EXPR_LT:
				if(*ip->un.src!=INFINITY)break;
			case EXPR_LE:
				if(*ip->un.src!=INFINITY&&
				*ip->un.src!=DBL_MAX)break;*/
			case EXPR_ANDL:
				if(fabs(*ip->un.src)<=DBL_EPSILON){
					ip->op=EXPR_CONST;
					ip->un.value=0.0;
					r=1;
				}
				break;
			//case EXPR_XORL:
			case EXPR_ORL:
				if(fabs(*ip->un.src)>DBL_EPSILON){
					ip->op=EXPR_CONST;
					ip->un.value=1.0;
					r=1;
				}
				break;
			default:
				ze=expr_zero_element(ip->op);
				if(ze<0.0
					||expr_modified(ep,ip->un.src)
					)continue;
				if(ze==*ip->un.src){
					ip->dst=NULL;
					r=1;
				}
				break;
		}
	}
	return r;
}
static void expr_optimize_const(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&!expr_modified(ep,ip->dst)){
			//*ip->dst=ip->un.value;
			//not necessary
			ip->dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
static int expr_side(enum expr_op op){
	switch(op){
		case EXPR_COPY:
		case EXPR_INPUT:
		case EXPR_CONST:
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
		case EXPR_POW:
		case EXPR_AND:
		case EXPR_OR:
		case EXPR_XOR:
		case EXPR_SHL:
		case EXPR_SHR:
		case EXPR_NEG:
		case EXPR_NOT:
		case EXPR_NOTL:
		case EXPR_GT:
		case EXPR_GE:
		case EXPR_LT:
		case EXPR_LE:
		case EXPR_SEQ:
		case EXPR_EQ:
		case EXPR_NE:
		case EXPR_ANDL:
		case EXPR_ORL:
		case EXPR_XORL:
			return 0;
		default:
			return 1;
	}
}
static int expr_override(enum expr_op op){
	switch(op){
		case EXPR_COPY:
		case EXPR_INPUT:
		case EXPR_CONST:
		case EXPR_IF:
		case EXPR_WHILE:
		case EXPR_SUM:
		case EXPR_INT:
		case EXPR_PROD:
		case EXPR_SUP:
		case EXPR_INF:
		case EXPR_ANDN:
		case EXPR_ORN:
		case EXPR_XORN:
		case EXPR_GCDN:
		case EXPR_LCMN:
		case EXPR_LOOP:
		case EXPR_FOR:
		case EXPR_CALLZA:
		case EXPR_CALLMD:
		case EXPR_CALLME:
			return 1;
		default:
			return 0;
	}
}
static int expr_usesrc(enum expr_op op){
	switch(op){
		case EXPR_COPY:
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
		case EXPR_POW:
		case EXPR_AND:
		case EXPR_OR:
		case EXPR_XOR:
		case EXPR_SHL:
		case EXPR_SHR:
		case EXPR_GT:
		case EXPR_GE:
		case EXPR_LT:
		case EXPR_LE:
		case EXPR_SEQ:
		case EXPR_EQ:
		case EXPR_NE:
		case EXPR_ANDL:
		case EXPR_ORL:
		case EXPR_XORL:
			return 1;
		default:
			return 0;
	}
}
static int expr_usesum(enum expr_op op){
	switch(op){
		case EXPR_SUM:
		case EXPR_INT:
		case EXPR_PROD:
		case EXPR_SUP:
		case EXPR_INF:
		case EXPR_ANDN:
		case EXPR_ORN:
		case EXPR_XORN:
		case EXPR_GCDN:
		case EXPR_LCMN:
		case EXPR_FOR:
		case EXPR_LOOP:
				return 1;
		default:
				return 0;
	}
}
static int expr_usemd(enum expr_op op){
	switch(op){
		case EXPR_CALLMD:
		case EXPR_CALLME:
				return 1;
		default:
				return 0;
	}
}
static int expr_usebranch(enum expr_op op){
	switch(op){
		case EXPR_IF:
		case EXPR_WHILE:
				return 1;
		default:
				return 0;
	}
}
static int expr_vused(struct expr_inst *ip1,double *v){
	int ov;
	for(;;++ip1){
		ov=expr_override(ip1->op);
		if((expr_usesrc(ip1->op)&&ip1->un.src==v)
			||(ip1->dst==v&&!ov)
		){
			return 1;
		}
		if(ip1->dst==v&&ov){
			return 0;
		}
		if(ip1->op==EXPR_END){
			return 0;
		}
	}
	return 0;
}
static int expr_vcheck_ep(struct expr_inst *ip0,double *v){
	if(expr_vused(ip0,v))return 1;
	for(struct expr_inst *ip=ip0;ip->op!=EXPR_END;++ip){
		if(expr_usesum(ip->op)&&(
			expr_vcheck_ep(ip->un.es->from->data,v)||
			expr_vcheck_ep(ip->un.es->to->data,v)||
			expr_vcheck_ep(ip->un.es->step->data,v)||
			expr_vcheck_ep(ip->un.es->ep->data,v)
			))return 1;
		if(expr_usebranch(ip->op)&&(
			expr_vcheck_ep(ip->un.eb->cond->data,v)||
			expr_vcheck_ep(ip->un.eb->body->data,v)||
			expr_vcheck_ep(ip->un.eb->value->data,v)
			))return 1;
		if(expr_usemd(ip->op)){
			for(size_t i=0;i<ip->un.em->dim;++i){
				if(expr_vcheck_ep(ip->un.em->eps[i].data,v))
					return 1;
			}
		}
		if(ip->op==EXPR_CALLHOT&&(
			expr_vcheck_ep(ip->un.hotfunc->data,v)
			))return 1;
	}
	return 0;
}

static void expr_optimize_unused(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){

		if(!expr_varofep(ep,ip->dst)
			||expr_side(ip->op))continue;
		if(/*!expr_used(ip)&&*/!expr_vcheck_ep(ip+1,ip->dst)){
			ip->dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_constneg(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_NEG:
			case EXPR_NOT:
			case EXPR_NOTL:
				break;
			default:
				continue;
		}
		for(struct expr_inst *ip1=ip-1;ip>=ep->data;--ip1){
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_CONST)break;
			switch(ip->op){
				case EXPR_NEG:
					ip1->un.value=-ip1->un.value;
					break;
				case EXPR_NOT:
					ip1->un.value=expr_not(ip1->un.value);
					break;
				case EXPR_NOTL:
					ip1->un.value=fabs(ip1->un.value)<=DBL_EPSILON?
						1.0:0.0;
					break;
				default:
					continue;
			}
			ip->dst=NULL;
			break;
		}
	}
	expr_optimize_completed(ep);
}

static int expr_injection_symtype(int type){
	switch(type){
		case EXPR_FUNCTION:
		case EXPR_ZAFUNCTION:
		//case EXPR_HOTFUNCTION:
			return 1;
		default:
			return 0;
	}
}
static int expr_injection_optype(enum expr_op op){
	switch(op){
		case EXPR_CALL:
		case EXPR_CALLZA:
		//case EXPR_CALLHOT: not work
			return 1;
		default:
			return 0;
	}
}
static int expr_isinjection(struct expr *restrict ep,struct expr_inst *ip){
	union {
		const struct expr_symbol *es;
		const struct expr_builtin_symbol *ebs;
	} sym;
	expr_injection_optype(ip->op);
	if(ep->sset
	&&(sym.es=expr_symset_rsearch(ep->sset,ip->un.func))){
		if(expr_injection_symtype(sym.es->type)
		&&(sym.es->flag&EXPR_SF_INJECTION))
			return 1;
	}else {
		if((sym.ebs=expr_bsym_rsearch(ip->un.func))
		&&expr_injection_symtype(sym.ebs->type)
		&&(sym.ebs->flag&EXPR_SF_INJECTION))
			return 1;
	}
	return 0;
}
static int expr_optimize_injection(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
			if(!expr_isinjection(ep,ip))continue;
			//printf("in %zd\n",ip-ep->data);
			if(ip->op==EXPR_CALLZA){
				ip->un.value=ip->un.zafunc();
			//printf("in %lf\n",ip->un.value);
				ip->op=EXPR_CONST;
				r=1;
				continue;
			}
		for(struct expr_inst *ip1=ip-1;ip1>=ep->data;--ip1){
			//printf("at [%zd] in %zd\n",ip1-ep->data,ip-ep->data);
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_CONST)break;
			switch(ip->op){
				case EXPR_CALL:
					ip1->un.value=ip->un.func(ip1->un.value);
					break;
				/*case EXPR_CALLHOT:
					ip1->un.value=expr_eval(ip->un.hotfunc,ip1->un.value);
					break;*/
				default:
					abort();
			}
			r=1;
			ip->dst=NULL;
			break;
		}
	}
	expr_optimize_completed(ep);
	return r;
}
static void expr_optimize_copyadd(struct expr *restrict ep){
	struct expr_inst *ip2;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY||
			!expr_varofep(ep,ip->dst)
			||expr_vcheck_ep(ip+1,ip->dst))continue;
		ip2=NULL;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst==ip->dst)goto fail;
			if(!expr_usesrc(ip1->op)||ip1->un.src!=ip->dst)continue;
			if(ip2){
fail:
				ip2=NULL;
				break;
			}else
				ip2=ip1;
		}
		if(ip2){
			ip2->un.src=ip->un.src;
			ip->dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
/*static int expr_optimize_copy2const(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY
			//||
			//!expr_varofep(ep,ip->dst)||
			//!expr_varofep(ep,ip->un.src)
			)continue;
		if(!expr_modified(ep,ip->un.src)){
			ip->op=EXPR_CONST;
			ip->un.value=*ip->un.src;
			r=1;
		}
	}
	expr_optimize_completed(ep);
	return r;
}*/
//a bug cannot fix
static void expr_optimize_copyend(struct expr *restrict ep){
	struct expr_inst *lip=NULL;
	struct expr_inst *ip=ep->data;
	for(;ip->op!=EXPR_END;++ip){
		lip=ip;
	}
	if(lip&&lip->op==EXPR_COPY&&lip->dst==ip->dst&&
		expr_varofep(ep,lip->dst)
		){
		ip->dst=lip->un.src;
		lip->dst=NULL;
	}
	expr_optimize_completed(ep);
}
static int expr_optimize_once(struct expr *restrict ep){
	int r=0;
	expr_optimize_const(ep);
	expr_optimize_constneg(ep);
	r|=expr_optimize_injection(ep);

	expr_optimize_contmul(ep,EXPR_POW);
	expr_optimize_contmul(ep,EXPR_MUL);
	expr_optimize_contmul(ep,EXPR_DIV);
	expr_optimize_contmul(ep,EXPR_MOD);
	expr_optimize_contadd(ep);
	expr_optimize_contsh(ep);
	expr_optimize_contmul(ep,EXPR_LT);
	expr_optimize_contmul(ep,EXPR_LE);
	expr_optimize_contmul(ep,EXPR_GT);
	expr_optimize_contmul(ep,EXPR_GE);
	expr_optimize_contmul(ep,EXPR_SEQ);
	expr_optimize_contmul(ep,EXPR_EQ);
	expr_optimize_contmul(ep,EXPR_NE);
	expr_optimize_contmul(ep,EXPR_AND);
	expr_optimize_contmul(ep,EXPR_XOR);
	expr_optimize_contmul(ep,EXPR_OR);
	expr_optimize_contmul(ep,EXPR_ANDL);
	expr_optimize_contmul(ep,EXPR_XORL);
	expr_optimize_contmul(ep,EXPR_ORL);
	//expr_optimize_contmul(ep,EXPR_COPY);

	r|=expr_optimize_zero(ep);
	//r|=expr_optimize_copy2const(ep);
	expr_optimize_copyadd(ep);
	expr_optimize_unused(ep);
	expr_optimize_copyend(ep);
	return r;
}
static void expr_optimize(struct expr *restrict ep){
	size_t s=ep->size;
	int r;
	expr_writeconsts(ep);
again:
	r=expr_optimize_once(ep);
	if((ep->size<s||r)&&ep->size>1){
		s=ep->size;
		goto again;
	}

}
double expr_eval(const struct expr *restrict ep,double input){
	double step,sum,from,to,y;
	int neg;
	for(struct expr_inst *ip=ep->data;;++ip){
		assert(ip->op>=0);
		assert(ip->op<=EXPR_END);
		switch(ip->op){
			case EXPR_COPY:
				*ip->dst=*ip->un.src;
				break;
			case EXPR_INPUT:
				*ip->dst=input;
				break;
			case EXPR_CALL:
				*ip->dst=ip->un.func(*ip->dst);
				break;
			case EXPR_CONST:
				*ip->dst=ip->un.value;
				break;
			case EXPR_ADD:
				*ip->dst+=*ip->un.src;
				break;
			case EXPR_SUB:
				*ip->dst-=*ip->un.src;
				break;
			case EXPR_MUL:
				*ip->dst*=*ip->un.src;
				break;
			case EXPR_DIV:
				*ip->dst/=*ip->un.src;
				break;
			case EXPR_MOD:
				*ip->dst=fmod(*ip->dst,*ip->un.src);
				break;
			case EXPR_POW:
				*ip->dst=pow(*ip->dst,*ip->un.src);
				break;
			case EXPR_AND:
				*ip->dst=expr_and2(*ip->dst,*ip->un.src);
				break;
			case EXPR_OR:
				*ip->dst=expr_or2(*ip->dst,*ip->un.src);
				break;
			case EXPR_XOR:
				*ip->dst=expr_xor2(*ip->dst,*ip->un.src);
				break;
			case EXPR_SHL:
				EXPR_EDEXP(ip->dst)+=(int64_t)*ip->un.src;
				break;
			case EXPR_SHR:
				EXPR_EDEXP(ip->dst)-=(int64_t)*ip->un.src;
				break;
			case EXPR_NEG:
				*ip->dst=-*ip->dst;
				break;
			case EXPR_NOT:
				*ip->dst=expr_not(*ip->dst);
				break;
			case EXPR_NOTL:
				*ip->dst=fabs(*ip->dst)<=DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_GT:
				*ip->dst=*ip->dst>*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_LT:
				*ip->dst=*ip->dst<*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_GE:
				*ip->dst=*ip->dst>=*ip->un.src-DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_LE:
				*ip->dst=*ip->dst<=*ip->un.src+DBL_EPSILON?
					1.0:
					0.0;
				break;

			case EXPR_SEQ:
				*ip->dst=*ip->dst==*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_EQ:
				*ip->dst=fabs(*ip->dst-*ip->un.src)<=DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_NE:
				*ip->dst=fabs(*ip->dst-*ip->un.src)>DBL_EPSILON?
					1.0:
					0.0;
				break;
//#define CALLOGIC(a,b,_s) ((fabs(a)>DBL_EPSILON) _s (fabs(b)>DBL_EPSILON))
			case EXPR_ANDL:
				*ip->dst=CALLOGIC(*ip->dst,*ip->un.src,&&)?
					1.0:
					0.0;
				break;
			case EXPR_ORL:
				*ip->dst=CALLOGIC(*ip->dst,*ip->un.src,||)?
					1.0:
					0.0;
				break;
			case EXPR_XORL:
				*ip->dst=CALLOGIC(*ip->dst,*ip->un.src,^)?
					1.0:
					0.0;
				break;
				
#define CALSUM(_op,_do,_init,_neg)\
			case _op :\
				neg=0;\
				from=expr_eval(ip->un.es->from,input);\
				to=expr_eval(ip->un.es->to,input);\
				if(from>to){\
					step=from;\
					from=to;\
					to=step;\
					neg=1;\
				}\
				step=expr_eval(ip->un.es->step,input);\
				if(step<0){\
					step=-step;\
					neg^=1;\
				}\
				_init ;\
				for(ip->un.es->index=from;\
					ip->un.es->index<=to;\
					ip->un.es->index+=step){\
					y=expr_eval(ip->un.es->ep,input) ;\
					_do ;\
				}\
				if(neg)sum= _neg ;\
				*ip->dst=sum;\
				break
			CALSUM(EXPR_SUM,sum+=y,sum=0.0,-sum);
			CALSUM(EXPR_INT,sum+=step*y,sum=0.0;from+=step/2.0,-sum);
			CALSUM(EXPR_PROD,sum*=y,sum=1.0,1.0/sum);
			CALSUM(EXPR_SUP,if(y>sum)sum=y,sum=DBL_MIN,sum);
			CALSUM(EXPR_INF,if(y<sum)sum=y,sum=DBL_MAX,sum);
			CALSUM(EXPR_ANDN,sum=sum!=DBL_MAX?expr_and2(sum,y):y,sum=DBL_MAX,sum);
			CALSUM(EXPR_ORN,sum=sum!=0.0?expr_or2(sum,y):y,sum=0.0,sum);
			CALSUM(EXPR_XORN,sum=sum!=0.0?expr_xor2(sum,y):y,sum=0.0,sum);
			CALSUM(EXPR_GCDN,sum=sum!=DBL_MAX?expr_gcd2(sum,y):y,sum=DBL_MAX,sum);
			CALSUM(EXPR_LCMN,sum=sum!=1.0?expr_lcm2(sum,y):y,sum=1.0,sum);

			case EXPR_FOR:
				ip->un.es->index=
				expr_eval(ip->un.es->from,input);//init
				to=expr_eval(ip->un.es->to,input);//cond
				if(to<0.0)to=-to;
				while(to>DBL_EPSILON){
					expr_eval(ip->un.es->step,input);//every time
					to=expr_eval(ip->un.es->to,input);//cond
					if(to<0.0)to=-to;
				}
				*ip->dst=expr_eval(ip->un.es->ep,input);
				break;
			case EXPR_LOOP:
				ip->un.es->index=
				expr_eval(ip->un.es->from,input);//init
				to=expr_eval(ip->un.es->to,input);//times
				if(to<0)to=-to;
				for(;to>DBL_EPSILON;to-=1.0){
					expr_eval(ip->un.es->step,input);//every time
				}
				*ip->dst=expr_eval(ip->un.es->ep,input);
				break;
			case EXPR_IF:
				*ip->dst=
				fabs(expr_eval(ip->un.eb->cond,input))>DBL_EPSILON?
				expr_eval(ip->un.eb->body,input):
				expr_eval(ip->un.eb->value,input);
				break;
			case EXPR_WHILE:
				while(fabs(expr_eval(ip->un.eb->cond,input))>DBL_EPSILON)
				expr_eval(ip->un.eb->body,input);
				*ip->dst=
				expr_eval(ip->un.eb->value,input);
				break;
			case EXPR_CALLZA:
				*ip->dst=ip->un.zafunc();
				break;
			case EXPR_CALLMD:
				for(size_t i=0;i<ip->un.em->dim;++i)
					ip->un.em->args[i]=
						expr_eval(
						ip->un.em->eps+i,input);
				
				*ip->dst=ip->un.em->un.func(ip->un.em->dim,ip->un.em->args);
				break;
			case EXPR_CALLME:
				*ip->dst=ip->un.em->un.funcep(ip->un.em->dim,ip->un.em->eps,input);
				break;
			case EXPR_CALLHOT:
				*ip->dst=expr_eval(ip->un.hotfunc,*ip->dst);
				break;
			case EXPR_END:
				return *ip->dst;
		}
	}
}
