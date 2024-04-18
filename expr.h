/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_
#include <stdint.h>
#include <stddef.h>
enum expr_op {
EXPR_COPY=0,
EXPR_INPUT,
EXPR_CONST,
EXPR_CALL,
EXPR_ADD,
EXPR_SUB,
EXPR_MUL,
EXPR_DIV,
EXPR_MOD,
EXPR_POW,
EXPR_AND,
EXPR_XOR,
EXPR_OR,
EXPR_SHL,
EXPR_SHR,
EXPR_GT,
EXPR_GE,
EXPR_LT,
EXPR_LE,
EXPR_SLE,
EXPR_SGE,
EXPR_SEQ,
EXPR_SNE,
EXPR_EQ,
EXPR_NE,
EXPR_ANDL,
EXPR_ORL,
EXPR_XORL,
EXPR_NEXT,
EXPR_NEG,
EXPR_NOT,
EXPR_NOTL,
EXPR_TSTL,
EXPR_IF,
EXPR_WHILE,
EXPR_SUM,
EXPR_INT,
EXPR_PROD,
EXPR_SUP,
EXPR_INF,
EXPR_ANDN,
EXPR_ORN,
EXPR_XORN,
EXPR_GCDN,
EXPR_LCMN,
EXPR_LOOP,
EXPR_FOR,
EXPR_ZA,
EXPR_MD,
EXPR_ME,
EXPR_VMD,
EXPR_HOT,
EXPR_END
};
#define EXPR_SYMSET_INITIALIZER {NULL,0UL,0UL,0UL,0}
#define EXPR_SYMLEN 64
#ifndef EXPR_SYMNEXT
#define EXPR_SYMNEXT 14
#endif
#define EXPR_ESYMBOL 1
#define EXPR_EPT 2
#define EXPR_EFP 3
#define EXPR_ENVP 4
#define EXPR_ENEA 5
#define EXPR_ENUMBER 6
#define EXPR_ETNV 7
#define EXPR_EEV 8
#define EXPR_EUO 9
#define EXPR_EZAFP 10
#define EXPR_EDS 11
#define EXPR_EVMD 12
#define EXPR_EMEM 13

#define EXPR_CONSTANT 0
#define EXPR_VARIABLE 1
#define EXPR_FUNCTION 2
#define EXPR_MDFUNCTION 3
#define EXPR_MDEPFUNCTION 4
#define EXPR_HOTFUNCTION 5
#define EXPR_ZAFUNCTION 6
//expr symbol flag
#define EXPR_SF_INJECTION 1
//expr initial flag
#define EXPR_IF_NOOPTIMIZE 1
#define EXPR_IF_INSTANT_FREE 2

#define EXPR_IF_EXTEND_MASK (\
		EXPR_IF_INSTANT_FREE)

#define EXPR_EDBASE(d) (((union expr_double *)(d))->rd.base)
#define EXPR_EDEXP(d) (((union expr_double *)(d))->rd.exp)
#define EXPR_EDSIGN(d) (((union expr_double *)(d))->rd.sign)
#define EXPR_EDIVAL(d) (((union expr_double *)(d))->ival)
struct expr;
struct expr_symset;
struct expr_suminfo {
	struct expr *from,*to,*step,*ep;
	volatile double index;
};
struct expr_branchinfo {
	struct expr *cond,*body,*value;
};
struct expr_mdinfo {
	struct expr *eps;
	double *args;
	union {
		double (*func)(size_t,double *);
		double (*funcep)(size_t,
			const struct expr *,double);
	} un;
	size_t dim;
};
struct expr_vmdinfo {
	struct expr *from,*to,*step,*ep;
	ssize_t max;
	double (*func)(size_t,double *);
	double *args;
	volatile double index;
};
union expr_inst_op2{
	double *src;
	void *uaddr;
	double value;
	struct expr *hotfunc;
	double (*func)(double);
	double (*zafunc)(void);
	struct expr_suminfo *es;
	struct expr_branchinfo *eb;
	struct expr_mdinfo *em;
	struct expr_vmdinfo *ev;
};
struct expr_inst {
	double *dst;
	union expr_inst_op2 un;
	enum expr_op op;
	int flag;
};
union expr_symvalue {
	double value;
	long ivalue;
	unsigned long uvalue;
	double *addr;
	void *uaddr;
	double (*func)(double);
	double (*zafunc)(void);
	char *hotexpr;
	double (*mdfunc)(size_t,double *);
	double (*mdepfunc)(size_t,
		const struct expr *,double);
};
//_Static_assert(sizeof(union expr_symvalue)==8,"symbol_value size error");
struct expr_symbol {
	union expr_symvalue un;
	struct expr_symbol *next[EXPR_SYMNEXT];
	unsigned int length;
	unsigned short strlen;
	char type,flag;
	char str[];
}/* __attribute__((packed))*/;
//_Static_assert(sizeof(struct expr_symbol)-EXPR_SYMNEXT*sizeof(struct expr_symbol *)==16,"symbol size error");
struct expr_builtin_symbol {
	union expr_symvalue un;
	const char *str;
	unsigned short strlen;
	short type,flag,dim;
};
struct expr_builtin_keyword {
	const char *str;
	enum expr_op op;
	unsigned short dim;
	unsigned short strlen;
	const char *desc;
};
struct expr_symset {
	struct expr_symbol *syms;
	size_t size,depth,length;
	int freeable;
};
struct expr {
	double **vars;
	struct expr_inst *data;
	struct expr_symset *sset;
	size_t size,length,vsize,vlength;
	int error;
	short iflag;
	char freeable,sset_shouldfree;
	char errinfo[EXPR_SYMLEN];
};
struct expr_rawdouble {
	uint64_t base:52;
	uint64_t exp:11;
	uint64_t sign:1;
} __attribute__((packed));
union expr_double {
	double val;
	uint64_t ival;
	struct expr_rawdouble rd;
};
extern const struct expr_builtin_symbol expr_bsyms[];
extern const struct expr_builtin_keyword expr_keywords[];
const char *expr_error(int error);
uint64_t expr_gcd64(uint64_t x,uint64_t y);
double expr_gcd2(double x,double y);
double expr_lcm2(double x,double y);
void expr_fry(double *v,size_t n);
void *expr_sort3(double *v,size_t n,void *(*allocator)(size_t));
void expr_sort_old(double *restrict v,size_t n);
void expr_sort(double *v,size_t n);
double expr_and2(double a,double b);
double expr_or2(double a,double b);
double expr_xor2(double a,double b);
double expr_multilevel_derivate(const struct expr *ep,double input,long level,double epsilon);
const struct expr_builtin_symbol *expr_bsym_search(const char *sym,size_t sz);
const struct expr_builtin_symbol *expr_bsym_rsearch(void *addr);
size_t expr_strscan(const char *s,size_t sz,char *buf);
char *expr_astrscan(const char *s,size_t sz,size_t *outsz);
void expr_free(struct expr *restrict ep);
struct expr_inst *expr_addop(struct expr *restrict ep,double *dst,void *src,enum expr_op op,int flag);
void init_expr_symset(struct expr_symset *restrict esp);
struct expr_symset *new_expr_symset(void);
void expr_symset_free(struct expr_symset *restrict esp);
void expr_symset_wipe(struct expr_symset *restrict esp);
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,...);
struct expr_symbol *expr_symset_addl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,...);
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,va_list ap);
struct expr_symbol *expr_symset_vaddl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,va_list ap);
struct expr_symbol *expr_symset_addcopy(struct expr_symset *restrict esp,const struct expr_symbol *restrict es);
struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz);
struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr);
void expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src);
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict ep);
int init_expr5(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp,int flag);
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp);
struct expr *new_expr7(const char *e,const char *asym,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]);
struct expr *new_expr6(const char *e,const char *asym,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]);
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]);
double expr_calc5(const char *e,const char *asym,double input,struct expr_symset *esp,int flag);
double expr_calc4(const char *e,const char *asym,double input,struct expr_symset *esp);
double expr_calc3(const char *e,const char *asym,double input);
double expr_calc(const char *e);
double expr_eval(const struct expr *restrict ep,double input);
#endif
