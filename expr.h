/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_
#define EXPR_COPY 0
#define EXPR_CALL 1
#define EXPR_ADD 2
#define EXPR_SUB 3
#define EXPR_MUL 4
#define EXPR_DIV 5
#define EXPR_POW 6
#define EXPR_NEG 7
#define EXPR_END 8
#define EXPR_SYMLEN 256
#define EXPR_ESYMBOL 1
#define EXPR_EPT 2
#define EXPR_EFP 3
#define EXPR_ENVP 4
struct expr_inst {
	double *dst;
	union {
		double *src;
		double (*func)(double);
	} un;
	unsigned int op;
};
struct expr_symbol {
	double (*addr)(double);
	char str[EXPR_SYMLEN];
};
struct expr_symset {
	struct expr_symbol *syms;
	size_t size,length;
};
struct expr {
	double *vars;;
	struct expr_inst *data;
	struct expr_symset *sset;
	size_t size,length,vsize,vlength;
	int error;
};
void expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double));
void expr_addop(struct expr *restrict ep,double *dst,double *src,unsigned int op);
#define expr_addcopy(e,t,f) expr_addop(e,t,f,EXPR_COPY)
#define expr_addadd(e,t,f) expr_addop(e,t,f,EXPR_ADD)
#define expr_addsub(e,t,f) expr_addop(e,t,f,EXPR_SUB)
#define expr_addmul(e,t,f) expr_addop(e,t,f,EXPR_MUL)
#define expr_adddiv(e,t,f) expr_addop(e,t,f,EXPR_DIV)
#define expr_addpow(e,t,f) expr_addop(e,t,f,EXPR_POW)
#define expr_addneg(e,t) expr_addop(e,t,NULL,EXPR_NEG)
#define expr_addend(e,t) expr_addop(e,t,NULL,EXPR_END)
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error);
double expr_compute(struct expr *restrict ep,double input);
void expr_free(struct expr *restrict ep);
const char *expr_error(int error);
void expr_symset_add(struct expr_symset *restrict ep,const char *sym,double (*addr)(double));
struct expr_symset *new_expr_symset(void);
void expr_symset_free(struct expr_symset *restrict esp);
#endif
