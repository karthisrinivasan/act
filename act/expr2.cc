/*************************************************************************
 *
 *  This file is part of the ACT library
 *
 *  Copyright (c) 2018-2025 Rajit Manohar
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *
 **************************************************************************
 */
#include <act/types.h>
#include <act/body.h>
#include <act/value.h>
#include <act/lang.h>
#include <act/act.h>
#include <common/int.h>
#include <string.h>
#include <string>
#include "expr_width.h"


static Expr *const_int_ex (long val)
{
  Expr *e;
  NEW (e, Expr);
  e->type = E_INT;
  e->u.ival.v = val;
  BigInt *b = new BigInt;
  b->setVal (0, val);
  b->setWidth (act_expr_intwidth (val));
  e->u.ival.v_extra = b;
  return e;
}


/*
  hash ids!
*/
//#define _id_equal(a,b) ((a) == (b))
//#define _id_equal ((ActId *)(a))->isEqual ((ActId *)(b))
static int _id_equal (const Expr *a, const Expr *b)
{
  ActId *ia, *ib;
  ia = (ActId *)a;
  ib = (ActId *)b;
  return ia->isEqual (ib);
}

static int expr_args_equal (const Expr *a, const Expr *b)
{
  while (a && b) {
    if (!expr_equal (a->u.e.l, b->u.e.l)) {
      return 0;
    }
    a = a->u.e.r;
    b = b->u.e.r;
  }
  if (a || b) {
    return 0;
  }
  return 1;
}

/**
 *  Compare two expressions structurally for equality
 *
 *  \param a First expression to be compared
 *  \param b Second expression to be compared
 *  \return 1 if the two are structurally identical, 0 otherwise
 */
int expr_equal (const Expr *a, const Expr *b)
{
  int ret;
  if (a == b) { 
    return 1;
  }
  if (a == NULL || b == NULL) {
    /* this means a != b given the previous clause */
    return 0;
  }
  if (a->type != b->type) {
    return 0;
  }
  switch (a->type) {
    /* binary */
  case E_AND:
  case E_OR:
  case E_PLUS:
  case E_MINUS:
  case E_MULT:
  case E_DIV:
  case E_MOD:
  case E_LSL:
  case E_LSR:
  case E_ASR:
  case E_XOR:
  case E_LT:
  case E_GT:
  case E_LE:
  case E_GE:
  case E_EQ:
  case E_NE:
    if (expr_equal (a->u.e.l, b->u.e.l) &&
	expr_equal (a->u.e.r, b->u.e.r)) {
      return 1;
    }
    else {
      return 0;
    }
    break;

    /* unary */
  case E_UMINUS:
  case E_NOT:
  case E_COMPLEMENT:
    return expr_equal (a->u.e.l, b->u.e.l);
    break;

    /* special */
  case E_QUERY:
    if (expr_equal (a->u.e.l, b->u.e.l) &&
	expr_equal (a->u.e.r->u.e.l, b->u.e.r->u.e.l) &&
	expr_equal (a->u.e.r->u.e.r, b->u.e.r->u.e.r)) {
      return 1;
    }
    else {
      return 0;
    }
    break;

  case E_CONCAT:
    fatal_error ("Should not be here");
    ret = 1;
    while (a && ret && a->type == E_CONCAT) {
      if (!b || b->type != E_CONCAT) return 0;
      ret = expr_equal (a->u.e.l, b->u.e.l);
      if (!ret) return 0;
      a = a->u.e.r;
      b = b->u.e.r;
    }
    return 1;
    break;

  case E_BITFIELD:
    if (_id_equal (a->u.e.l, b->u.e.l) &&
	(a->u.e.r->u.e.l == b->u.e.r->u.e.l) &&
	(a->u.e.r->u.e.r == b->u.e.r->u.e.r)) {
      return 1;
    }
    else {
      return 0;
    }
    break;


  case E_ANDLOOP:
  case E_ORLOOP:
  case E_PLUSLOOP:
  case E_MULTLOOP:
  case E_XORLOOP:
    if (strcmp ((char *)a->u.e.l->u.e.l,
		(char *)b->u.e.l->u.e.l) != 0) {
      return 0;
    }
    if (!expr_equal (a->u.e.r->u.e.l, b->u.e.r->u.e.l)) {
      return 0;
    }
    if (!expr_equal (a->u.e.r->u.e.r->u.e.l, b->u.e.r->u.e.r->u.e.l)) {
      return 0;
    }
    if (!expr_equal (a->u.e.r->u.e.r->u.e.r, b->u.e.r->u.e.r->u.e.r)) {
      return 0;
    }
    return 1;
    break;

  case E_BUILTIN_BOOL:
  case E_BUILTIN_INT:
    if (!expr_equal (a->u.e.l, b->u.e.l)) {
      return 0;
    }
    if (!expr_equal (a->u.e.r, b->u.e.r)) {
      return 0;
    }
    return 1;
    break;
    
  case E_FUNCTION:
  case E_USERMACRO:
    if (a->u.fn.s != b->u.fn.s) return 0;
    {
      Expr *at, *bt;
      at = a->u.fn.r;
      bt = b->u.fn.r;

      if (at == NULL && bt == NULL) {
	return 1;
      }
      if (at == NULL || bt == NULL) {
	return 0;
      }

      if (at->type != bt->type) {
	return 0;
      }

      if (a->type == E_USERMACRO) {

	// built-in macros are a special case
	if (((UserMacro *)a->u.fn.s)->isBuiltinMacro()) {
	  if (((UserMacro *)a->u.fn.s)->isBuiltinStructMacro ()) {
	    if (at->type == E_GT) {
	      if (!expr_args_equal (at->u.e.l, bt->u.e.l)) {
		return 0;
	      }
	      at = at->u.e.r;
	      bt = bt->u.e.r;
	    }
	    Assert (at->type == E_LT, "What?");
	    Assert (bt->type == E_LT, "What?");
	    Assert (!at->u.e.r, "What?");
	    Assert (!bt->u.e.r, "What?");
	    return expr_equal (at->u.e.l, bt->u.e.l);
	  }
	  else {
	    return expr_equal (at, bt);
	  }
	}
	if (at->type == E_LT) {
	  if (!_id_equal (at->u.e.l, bt->u.e.l)) {
	    return 0;
	  }
	  at = at->u.e.r;
	  bt = bt->u.e.r;
	}
	else {
	  Assert (at->type == E_GT, "What?");
	  if (!expr_args_equal (at->u.e.l, bt->u.e.l)) {
	    return 0;
	  }
	  if (!_id_equal (at->u.e.r->u.e.l, bt->u.e.r->u.e.l)) {
	    return 0;
	  }
	  at = at->u.e.r->u.e.r;
	  bt = bt->u.e.r->u.e.r;
	}
	return expr_args_equal (at, bt);
      }
      else {
	if (at->type == E_LT) {
	  return expr_args_equal (at, bt);
	}
	else {
	  if (!expr_args_equal (at->u.e.l, bt->u.e.l)) {
	    return 0;
	  }
	  if (!expr_args_equal (at->u.e.r, bt->u.e.r)) {
	    return 0;
	  }
	  return 1;
	}
      }
    }
    break;

    /* leaf */
  case E_INT:
    if (a->u.ival.v == b->u.ival.v) {
      return 1;
    }
    else {
      return 0;
    }
    break;

  case E_REAL:
    if (a->u.f == b->u.f) {
      return 1;
    }
    else {
      return 0;
    }
    break;

  case E_VAR:
  case E_PROBE:
    /* hash the id */
    if (_id_equal (a->u.e.l, b->u.e.l)) {
      return 1;
    }
    else {
      return 0;
    }
    break;

  case E_TRUE:
  case E_FALSE:
    return 1;
    break;

  case E_ARRAY:
  case E_SUBRANGE:
    {
      ValueIdx *vxa = (ValueIdx *) a->u.e.l;
      ValueIdx *vxb = (ValueIdx *) b->u.e.l;
      Scope *sa, *sb;
      Arraystep *asa, *asb;
      int type;
      
      if (a->type == E_ARRAY) {
	sa = (Scope *) a->u.e.r;
	sb = (Scope *) b->u.e.r;
	asa = vxa->t->arrayInfo()->stepper();
	asb = vxb->t->arrayInfo()->stepper();
      }
      else {
	sa = (Scope *) a->u.e.r->u.e.l;
	sb = (Scope *) b->u.e.r->u.e.l;
	asa = vxa->t->arrayInfo()->stepper ((Array *)a->u.e.r->u.e.r);
	asb = vxb->t->arrayInfo()->stepper ((Array *)b->u.e.r->u.e.r);
      }

      if (TypeFactory::isPIntType (vxa->t)) {
	if (!TypeFactory::isPIntType (vxb->t)) {
	  delete asa;
	  delete asb;
	  return 0;
	}
	type = 0;
      }
      else if (TypeFactory::isPRealType (vxa->t)) {
	if (!TypeFactory::isPRealType (vxb->t)) {
	  delete asa;
	  delete asb;
	  return 0;
	}
	type = 1;
      }
      else if (TypeFactory::isPBoolType (vxa->t)) {
	if (!TypeFactory::isPBoolType (vxb->t)) {
	  delete asa;
	  delete asb;
	  return 0;
	}
	type = 2;
      }
      else if (TypeFactory::isPIntsType (vxa->t)) {
	if (!TypeFactory::isPIntsType (vxb->t)) {
	  delete asa;
	  delete asb;
	  return 0;
	}
	type = 3;
      }
      else {
	Assert (0, "E_ARRAY/SUBARRAY on non-params?");
      }
      while (!asa->isend()) {
	if (asb->isend()) {
	  delete asa;
	  delete asb;
	  return 0;
	}
	if (type == 0) {
	  Assert (sa->issetPInt (vxa->u.idx + asa->index()), "Hmm");
	  Assert (sb->issetPInt (vxb->u.idx + asb->index()), "Hmm");
	  if (sa->getPInt (vxa->u.idx + asa->index()) !=
	      sb->getPInt (vxb->u.idx + asb->index())) {
	    delete asa;
	    delete asb;
	    return 0;
	  }
	}
	else if (type == 1) {
	  Assert (sa->issetPReal (vxa->u.idx + asa->index()), "Hmm");
	  Assert (sb->issetPReal (vxb->u.idx + asb->index()), "Hmm");
	  if (sa->getPReal (vxa->u.idx + asa->index()) !=
	      sb->getPReal (vxb->u.idx + asb->index())) {
	    delete asa;
	    delete asb;
	    return 0;
	  }
	}
	else if (type == 2) {
	  Assert (sa->issetPBool (vxa->u.idx + asa->index()), "Hmm");
	  Assert (sb->issetPBool (vxb->u.idx + asb->index()), "Hmm");
	  if (sa->getPBool (vxa->u.idx + asa->index()) !=
	      sb->getPBool (vxb->u.idx + asb->index())) {
	    delete asa;
	    delete asb;
	    return 0;
	  }
	}
	else if (type == 3) {
	  Assert (sa->issetPInts (vxa->u.idx + asa->index()), "Hmm");
	  Assert (sb->issetPInts (vxb->u.idx + asb->index()), "Hmm");
	  if (sa->getPInts (vxa->u.idx + asa->index()) !=
	      sb->getPInts (vxb->u.idx + asb->index())) {
	    delete asa;
	    delete asb;
	    return 0;
	  }
	}
	asa->step();
	asb->step();
      }
      if (!asb->isend()) {
	delete asa;
	delete asb;
	return 0;
      }
      delete asa;
      delete asb;
      return 1;
    }
    break;
    
  default:
    fatal_error ("Unknown expression type?");
    return 0;
    break;
  }
  return 0;
}

int _act_chp_is_synth_flag = 0;

static void _eval_function (ActNamespace *ns, Scope *s, Expr *fn, Expr **ret,
			    int flags)
{
  Function *x = dynamic_cast<Function *>((UserDef *)fn->u.fn.s);
  Expr *e, *f;
  Assert (x, "What?");

  act_error_push (x->getName(), x->getFile(), x->getLine());
  if (!TypeFactory::isParamType (x->getRetType())) {
    /* ok just expand the arguments */
    Function *xf;
    xf = x->Expand (ns, s, 0, NULL);
    (*ret)->u.fn.s = (char *) ((UserDef *)xf);
    (*ret)->u.fn.r = NULL;
    e = fn->u.fn.r;
    f = NULL;
    while (e) {
      Expr *x = expr_expand (e->u.e.l, ns, s, flags);
      if (f == NULL) {
	NEW (f, Expr);
	(*ret)->u.fn.r = f;
	f->u.e.r = NULL;
      }
      else {
	NEW (f->u.e.r, Expr);
	f = f->u.e.r;
	f->u.e.r = NULL;
      }
      f->u.e.l = x;
      f->type = e->type;
      e = e->u.e.r;
    }
  }
  else {
    // elaborate function: parameterized type!
    Expr **args;
    int nargs;
    Expr *e;
    int skip_first = 0;

    nargs = 0;
    e = fn->u.e.r;
    while (e) {
      nargs++;
      e = e->u.e.r;
    }

    if (x->isUserMethod()) {
      skip_first = 1;
      nargs--;
    }
    
    if (nargs > 0) {
      MALLOC (args, Expr *, nargs);
      e = fn->u.e.r;
      if (skip_first) {
	e = e->u.e.r;
      }
      for (int i=0; i < nargs; i++) {
	args[i] = expr_expand (e->u.e.l, ns, s, flags);
	e = e->u.e.r;
      }
    }
    for (int i=0; i < nargs; i++) {
      if (!expr_is_a_const (args[i])) {
	if (args[i]->type == E_ARRAY) {
	  act_error_ctxt (stderr);
	  fatal_error ("Arrays are not supported as function arguments.");
	}
	else {
	  act_error_ctxt (stderr);
	  Assert (expr_is_a_const (args[i]), "Argument is not a constant?");
	}
      }
    }
    e = x->eval (ns, nargs, args);
    FREE (*ret);
    *ret = e;
    if (nargs > 0) {
      FREE (args);
    }
  }
  act_error_pop ();
  return;
}
    

/*------------------------------------------------------------------------
 * Expand expression, replacing all paramters. If it is an lval, then
 * it must be a pure identifier at the end of of the day. Default is
 * not an lval
 *
 *   pc = 1 means partial constant propagation is used
 *------------------------------------------------------------------------
 */
#if 0
static BigInt *_int_const (unsigned long v)
{
  BigInt *btmp;

  /* compute bitwidth */
  int w = _int_width (v);
  
  btmp = new BigInt (w, 0, 1);
  btmp->setVal (0, v);
  btmp->setWidth (w);

  return btmp;
}
#endif


static Expr *_expr_const_canonical (Expr *e, unsigned int flags)
{
  if (!(flags & ACT_EXPR_EXFLAG_PREEXDUP)) {
    Expr *tmp = TypeFactory::NewExpr (e);
    FREE (e);
    e = tmp;
  }
  return e;
}

static Expr *_expr_expand (int *width, Expr *e,
			   ActNamespace *ns, Scope *s, unsigned int flags)
{
  Expr *ret, *te;
  ActId *xid;
  Expr *tmp;
  int pc;
  int lw, rw;
  Expr *e_orig = NULL;
  
  if (!e) return NULL;

  NEW (ret, Expr);
  ret->type = e->type;
  ret->u.e.l = NULL;
  ret->u.e.r = NULL;

  pc = (flags & ACT_EXPR_EXFLAG_PARTIAL) ? 1 : 0;

#define LVAL_ERROR							\
    do {								\
      if (flags & ACT_EXPR_EXFLAG_ISLVAL) {				\
	act_error_ctxt (stderr);					\
	fprintf (stderr, "\texpanding expr: ");				\
	print_expr (stderr, e);						\
	fprintf (stderr, "\n");						\
	fatal_error ("Invalid assignable or connectable value!");	\
      }									\
    } while (0)

  if (e->type == E_USERMACRO) {
    UserMacro *um;
    UserDef *u;
    Expr *etmp;
    um = (UserMacro *) e->u.fn.s;
    if (flags & ACT_EXPR_EXFLAG_PREEXDUP) {
      Expr *tmp, *prev;
      Expr *w = e->u.fn.r;
      /*-- pure pre-ex dup --*/
      ret->u.fn.s = e->u.fn.s;

      if (um->isBuiltinMacro()) {
	int dummy;
	if (um->isBuiltinStructMacro ()) {
	  if (w) {
	    NEW (ret->u.fn.r, Expr);
	    ret->u.fn.r->type = w->type;
	    ret->u.fn.r->u.e.l = _expr_expand (&dummy, w->u.e.l, ns, s, flags);
	    ret->u.fn.r->u.e.r = NULL;
	    // we've either copied the first argument or the
	    // templates!
	    w = w->u.e.r;
	    tmp = ret->u.fn.r;
	    while (w) {
	      Assert (w->type == E_LT, "What?");
	      NEW (tmp->u.e.r, Expr);
	      tmp = tmp->u.e.r;
	      tmp->u.e.r = NULL;
	      tmp->type = E_LT;
	      tmp->u.e.l = _expr_expand (&dummy, w->u.e.l, ns, s, flags);
	      w = w->u.e.r;
	    }
	  }
	  else {
	    // this should not happen... but since this is predup this
	    // could be before any checks are done
	    ret->u.fn.r = NULL;
	  }
	}
	else {
	  ret->u.fn.r = _expr_expand (&dummy, e->u.fn.r, ns, s, flags);
	}
      }
      else {
	ret->u.fn.r = NULL;
	prev = NULL;
	while (w) {
	  int dummy;
	  NEW (tmp, Expr);
	  tmp->type = E_LT;
	  tmp->u.e.l = _expr_expand (&dummy, w->u.e.l, ns, s, flags);
	  tmp->u.e.r = NULL;
	  if (!prev) {
	    ret->u.fn.r = tmp;
	  }
	  else {
	    prev->u.e.r = tmp;
	  }
	  prev = tmp;
	  w = w->u.e.r;
	}
      }
      *width = -1;
      return ret;
    }
    /*
      Otherwise, expand it out and replace the usermacro and change
      it to a function!
    */
    ret->type = E_FUNCTION;
    /* um is the unexpanded macro; get the expanded one by looking up
       the type! */


    Expr *lexp;
    if (um->isBuiltinMacro()) {
      // We need to get the expanded user-defined type here!
      // int(...) or struct<...>(int)
      // case 1: we need the expanded type for the argument
      // case 2: we need to call the userdef-expand method on that type
      if (um->Parent()->isExpanded()) {
	// XXX: macro templates
	um = um->Parent()->getMacro (um->getName());
      }
      else {
	if (um->isBuiltinStructMacro()) {
	  UserDef *ux;
	  inst_param *inst;
	  int count = 0;
	  Expr *w;
	  // for "struct<...>(.)": the template parameters and struct
	  // need to just be expanded to get the macro information.
	  // this looks like a regular templated function with one
	  // argument.
	  Assert (e->u.fn.r, "Function from struct(..) macro has no args?");
	  ux = um->Parent();
	  if (e->u.fn.r->type == E_GT) {
	    /* Template parameters exist, extract them! */
	    w = e->u.fn.r->u.e.l;
	    count = 0;
	    while (w) {
	      count++;
	      w = w->u.e.r;
	    }
	    MALLOC (inst, inst_param, count);
	    w = e->u.fn.r->u.e.l;
	    for (int i=0; i < count; i++) {
	      AExpr *tae;
	      inst[i].isatype = 0;
	      tae = new AExpr (expr_dup (w->u.e.l));
	      inst[i].u.tp = tae->Expand (ns, s, 0);
	      delete tae;
	      w = w->u.e.r;
	    }
	  }
	  else {
	    inst = NULL;
	  }
	  ux = ux->Expand (ns, s, count, inst);
	  for (int i=0; i < count; i++) {
	    delete inst[i].u.tp;
	  }
	  if (count > 0) {
	    FREE (inst);
	  }
	  um = ux->getMacro (um->getName());
	}
	else {
	  // for "int(.)": we just need the structure return type. The
	  // argument MUST BE either a function or an ID.
	  // this looks like a normal function
	  Expr *exp = _expr_expand (&lw, e->u.fn.r, ns, s, flags);
	  Data *dx = NULL;
	  int errcode = -1;
	  Assert (exp, "What?");
	  dx = act_expr_is_structure (s, exp, &errcode);
	  Assert (errcode != 1 && errcode != 4 && errcode != 5,
		  "Unexpected structure expression");
	  if (errcode == 2) {
	    act_error_ctxt (stderr);
	    fprintf (stderr, "\texpanding expr: ");
	    print_expr (stderr, e);
	    fprintf (stderr,"\n");
	    fatal_error ("Could not find type for identifier? Should have been caught earlier!");
	  }
	  else if (errcode == 3) {
	    act_error_ctxt (stderr);
	    fprintf (stderr, "\texpanding expr: ");
	    print_expr (stderr, e);
	    fprintf (stderr,"\n");
	    fatal_error ("Function doesn't return a structure? Should have been caught earlier!");
	  }
	  Assert (dx, "Unexpected structure expression");
	  Assert (dx->isExpanded(), "What?");
	  um = dx->getMacro ("int");
	}
      }
      Assert (um->getFunction(), "What?");
      lexp = e->u.fn.r->u.e.l;
    }
    else {
      /* macro invocation: <id>.macro(...) */
      InstType *id_it;
      lexp = _expr_expand (&lw, e->u.fn.r->u.e.l, ns, s, flags);
      id_it = act_expr_insttype (s, lexp, NULL, 2);
      Assert (id_it, "Macro error!");
      UserDef *id_ux = dynamic_cast <UserDef *> (id_it->BaseType());
      Assert (id_ux, "What?");
      um = id_ux->getMacro (um->getName());
      Assert (um, "Expanded macro?");
      Assert (um->getFunction(), "What?");
    }

    e_orig = e;
    NEW (e, Expr);
    e->type = E_FUNCTION;
    /* XXX: replace e->u.fn.s */
    e->u.fn.s = (char *) um->getFunction ();

    if (um->isBuiltinMacro()) {
      if (um->isBuiltinStructMacro()) {
	NEW (e->u.fn.r, Expr);
	e->u.fn.r->type = E_LT;
	e->u.fn.r->u.e.r = NULL;
	if (e_orig->u.fn.r->type == E_GT) {
	  e->u.fn.r->u.e.l = e_orig->u.fn.r->u.e.r->u.e.l;
	}
	else {
	  e->u.fn.r->u.e.l = e_orig->u.fn.r->u.e.l;
	}
      }
      else {
	// int(...)
	NEW (e->u.fn.r, Expr);
	e->u.fn.r->type = E_LT;
	e->u.fn.r->u.e.l = e_orig->u.fn.r;
	e->u.fn.r->u.e.r = NULL;
      }
    }
    else {
      //Assert (e_orig->u.fn.r->u.e.l->type == E_VAR, "What?");
      //etmp = act_expr_var (((ActId
      //*)e_orig->u.fn.r->u.e.l->u.e.l)->Clone());
      etmp = expr_dup (lexp);
      NEW (e->u.fn.r, Expr);
      e->u.fn.r->type = E_LT;
      e->u.fn.r->u.e.l = etmp;
      e->u.fn.r->u.e.r = e_orig->u.fn.r->u.e.r;
    }
  }
  else if (e->type == E_BITSLICE) {
    if (flags & ACT_EXPR_EXFLAG_PREEXDUP) {
      Expr *tmp;
      ret->u.e.l = (Expr *)((ActId *)e->u.e.l)->Clone ();
      NEW (ret->u.e.r, Expr);
      tmp = ret->u.e.r;
      tmp->type = E_LT;
      tmp->u.e.l = _expr_expand (&lw, e->u.e.r->u.e.l, ns, s, flags);
      NEW (tmp->u.e.r, Expr);
      tmp = tmp->u.e.r;
      tmp->type = E_LT;
      tmp->u.e.l = _expr_expand (&lw, e->u.e.r->u.e.r->u.e.l, ns, s, flags);
      tmp->u.e.r = _expr_expand (&lw, e->u.e.r->u.e.r->u.e.r, ns, s, flags);
      return ret;
    }
    // re-write this as a concatenation
    // get the fields b_field and a_field.
    Expr *b_field, *a_field;
    b_field = _expr_expand (&lw, e->u.e.r->u.e.r->u.e.l, ns, s, flags);
    a_field = _expr_expand (&lw, e->u.e.r->u.e.r->u.e.r, ns, s, flags);
    if (!expr_is_a_const (b_field) || b_field->type != E_INT) {
      act_error_ctxt (stderr);
      fprintf (stderr, "\texpanding bit-slice expression: ");
      print_expr (stderr, e->u.e.r->u.e.r->u.e.l);
      fprintf (stderr, "\n");
      fatal_error ("Doesn't evaluate to an int constant!\n");
    }
    if (a_field && (!expr_is_a_const (a_field) || a_field->type != E_INT)) {
      act_error_ctxt (stderr);
      fprintf (stderr, "\texpanding bit-slice expression: ");
      print_expr (stderr, e->u.e.r->u.e.r->u.e.r);
      fprintf (stderr, "\n");
      fatal_error ("Doesn't evaluate to a int constant!\n");
    }
    if (!(flags & ACT_EXPR_EXFLAG_CHPEX)) {
      act_error_ctxt (stderr);
      fatal_error ("Bit-slice expression can only appear in CHP/dataflow bodies.");
    }
    ActId *xid = ((ActId *)e->u.e.l)->ExpandCHP (ns, s);
    Expr *te = xid->EvalCHP (ns, s, 0);
    if (te->type != E_VAR) {
      act_error_ctxt (stderr);
      fatal_error ("Not sure why this happened? Bit-slice id doesn't evaluate to a variable!");
    }
    act_chp_macro_check (s, (ActId *)te->u.e.l);
    InstType *it;
    act_type_var (s, (ActId *)te->u.e.l, &it);
    *width = TypeFactory::bitWidth (it);
    // now we have the bit-width!
    // now we convert the rest of the expression!!!
    int b_val, a_val;
    b_val = b_field->u.ival.v;
    if (a_field) {
      a_val = a_field->u.ival.v;
    }
    else {
      a_val = b_val;
    }
    if (b_val >= *width) {
      warning ("Bit-field assignment {%d..%d} exceeds integer width (%d); truncating", b_val, a_val, *width);
      b_val = (*width) - 1;
    }
    if (b_val < a_val) {
      act_error_ctxt (stderr);
      fatal_error ("Bit-field assignment {%d..%d} is empty.", b_val, a_val);
    }
    ret->type = E_CONCAT;
    // { opt- x{w-1 .. b+1} , int (E, b - a + 1), opt-x {a-1 .. 0} }
    if (b_val == (*width-1) && a_val == 0) {
      FREE (ret);
      return _expr_expand (width, e->u.e.r->u.e.l, ns, s, flags);
    }
    a_field = NULL;
    // optional x{w-1 .. b+1}
    if (b_val != (*width - 1)) {
      if (a_field) {
	NEW (a_field->u.e.r, Expr);
	a_field = a_field->u.e.r;
	a_field->type = E_CONCAT;
	a_field->u.e.r = NULL;
      }
      else {
	a_field = ret;
      }
      NEW (a_field->u.e.l, Expr);
      a_field->u.e.l->type = E_BITFIELD;
      a_field->u.e.l->u.e.l = (Expr *) ((ActId *)te->u.e.l)->Clone ();
      NEW (a_field->u.e.l->u.e.r, Expr);
      a_field->u.e.l->u.e.r->type = E_BITFIELD;
      a_field->u.e.l->u.e.r->u.e.l = const_expr (b_val+1);
      a_field->u.e.l->u.e.r->u.e.r = const_expr (*width - 1);
    }
    // int(E, b-a+1)
    if (a_field) {
      NEW (a_field->u.e.r, Expr);
      a_field = a_field->u.e.r;
      a_field->type = E_CONCAT;
      a_field->u.e.r = NULL;
    }
    else {
      a_field = ret;
    }
    NEW (a_field->u.e.l, Expr);
    a_field->u.e.l->type = E_BUILTIN_INT;
    a_field->u.e.l->u.e.l =
      _expr_expand (&lw, e->u.e.r->u.e.l, ns, s, flags);
    a_field->u.e.l->u.e.r = const_expr (b_val - a_val + 1);

    // opt x{a-1 .. 0}
    if (a_val != 0) {
      NEW (a_field->u.e.r, Expr);
      a_field = a_field->u.e.r;
      a_field->type = E_CONCAT;
      a_field->u.e.r = NULL;
      NEW (a_field->u.e.l, Expr);
      a_field->u.e.l->type = E_BITFIELD;
      a_field->u.e.l->u.e.l = (Expr *) ((ActId *)te->u.e.l)->Clone ();
      NEW (a_field->u.e.l->u.e.r, Expr);
      a_field->u.e.l->u.e.r->type = E_BITFIELD;
      a_field->u.e.l->u.e.r->u.e.l = const_expr (0);
      a_field->u.e.l->u.e.r->u.e.r = const_expr (a_val - 1);
    }
    return ret;
  }

  switch (e->type) {
  case E_ANDLOOP:
  case E_ORLOOP:
  case E_PLUSLOOP:
  case E_MULTLOOP:
  case E_XORLOOP:
    LVAL_ERROR;
    *width = 1;
    {
      int ilo, ihi;
      ValueIdx *vx;
      Expr *cur = NULL;

      act_syn_loop_setup (ns, s, (char *)e->u.e.l->u.e.l,
			  e->u.e.r->u.e.l, e->u.e.r->u.e.r->u.e.l,
			  &vx, &ilo, &ihi);

      int is_const = 1;
      int count = 0;
      int i;

      if (e->type == E_ANDLOOP) {
	ret->type = E_AND;
      }
      else if (e->type == E_ORLOOP) {
	ret->type = E_OR;
      }
      else if (e->type == E_PLUSLOOP) {
	ret->type = E_PLUS;
      }
      else if (e->type == E_MULTLOOP) {
	ret->type = E_MULT;
	*width = 0;
      }
      else if (e->type == E_XORLOOP) {
	ret->type = E_XOR;
      }

      for (i=ilo; i <= ihi; i++) {
	s->setPInt (vx->u.idx, i);
	Expr *tmp = _expr_expand (&lw, e->u.e.r->u.e.r->u.e.r, ns, s, flags);
	if (is_const && expr_is_a_const (tmp) &&
	    (tmp->type == E_TRUE || tmp->type == E_FALSE)) {
	  if (e->type == E_ANDLOOP) {
	    if (tmp->type == E_FALSE) {
	      /* we're done, the whole thing is false */
	      break;
	    }
	  }
	  else {
	    if (e->type != E_ORLOOP) {
	      act_error_ctxt (stderr);
	      fprintf (stderr, "\texpanding expr: ");
	      print_expr (stderr, e);
	      fprintf (stderr,"\n");
	      fatal_error ("Incompatible expression for loop operator");
	    }
	    if (tmp->type == E_TRUE) {
	      break;
	    }
	  }
	}
	else {
	  is_const = 0;
	  if (count == 0) {
	    ret->u.e.l = tmp;
	  }
	  else if (count == 1) {
	    ret->u.e.r = tmp;
	    cur = ret;
	  }
	  else {
	    Expr *tmp2;
	    NEW (tmp2, Expr);
	    tmp2->type = cur->type;
	    if (e->type == E_PLUSLOOP) {
	      tmp2->u.e.l = cur;
	      tmp2->u.e.r = tmp;
	      cur = tmp2;
	      ret = cur;
	    }
	    else {
	      tmp2->u.e.l = cur->u.e.r;
	      cur->u.e.r = tmp2;
	      tmp2->u.e.r = tmp;
	      cur = tmp2;
	    }
	  }
	  if (e->type == E_ANDLOOP || e->type == E_ORLOOP ||
	      e->type == E_XORLOOP) {
	    if (lw > *width) {
	      *width = lw;
	    }
	  }
	  else if (e->type == E_MULTLOOP) {
	    *width += lw;
	  }
	  else if (e->type == E_PLUSLOOP) {
	    *width = MAX(*width, lw)+1;
	  }
	  count++;
	}
      }
      if (is_const) {
	if (i <= ihi) {
	  /* short circuit */
	  if (e->type == E_ANDLOOP) {
	    ret->type = E_FALSE;
	  }
	  else {
	    ret->type = E_TRUE;
	  }
	  ret = _expr_const_canonical (ret, flags);
	}
	else {
	  if (e->type == E_ANDLOOP) {
	    int t = act_type_expr (s, e->u.e.r->u.e.r->u.e.r,
				   width, 2);
	    if (T_BASETYPE_BOOL (t)) {
	      // if this is an integer type?
	      ret->type = E_TRUE;
	      ret = _expr_const_canonical (ret, flags);
	    }
	    else {
	      ret->type = E_INT;
	      if (*width < 64) {
		ret->u.ival.v = (1UL << *width) - 1;
	      }
	      else {
		ret->u.ival.v = ~0UL;
	      }
	      if (flags & ACT_EXPR_EXFLAG_CHPEX) {
		BigInt *x = new BigInt;
		x->setWidth (*width);
		*x = ~*x;
		ret->u.ival.v_extra = x;
	      }
	      else {
		ret->u.ival.v_extra = NULL;
		ret = _expr_const_canonical (ret, flags);
	      }
	    }
	  }
	  else if (e->type == E_ORLOOP) {
	    int t = act_type_expr (s, e->u.e.r->u.e.r->u.e.r,
				   width, 2);
	    if (T_BASETYPE_BOOL (t)) {
	      ret->type = E_FALSE;
	      ret = _expr_const_canonical (ret, flags);
	    }
	    else {
	      ret->type = E_INT;
	      ret->u.ival.v = 0;
	      ret->u.ival.v_extra = NULL;
	      if (flags & ACT_EXPR_EXFLAG_CHPEX) {
		BigInt *x = new BigInt;
		x->setWidth (*width);	
		ret->u.ival.v_extra = x;
	      }
	      else {
		ret = _expr_const_canonical (ret, flags);
	      }
	    }
	  }
	  else {
	    // must be an empty loop
	    Assert (ilo > ihi, "What?");
	    int t = act_type_expr (s, e->u.e.r->u.e.r->u.e.r,
				   width, 2);
	    ret->type = E_INT;
	    ret->u.ival.v = 0;
	    ret->u.ival.v_extra = NULL;
	    BigInt *x = new BigInt;
	    x->setWidth (*width);
	    if (e->type == E_MULTLOOP) {
	      ret->u.ival.v = 1;
	      x->setVal (0, 1);
	      if (flags & ACT_EXPR_EXFLAG_CHPEX) {
		ret->u.ival.v_extra = x;
	      }
	      else {
		ret->u.ival.v_extra = NULL;
		ret = _expr_const_canonical (ret, flags);
		delete x;
	      }
	    }
	    else if (e->type == E_PLUSLOOP || e->type == E_XORLOOP) {
	      ret->u.ival.v = 0;
	      if (flags & ACT_EXPR_EXFLAG_CHPEX) {
		ret->u.ival.v_extra = x;
	      }
	      else {
		ret->u.ival.v_extra = NULL;
		ret = _expr_const_canonical (ret, flags);
		delete x;
	      }
	    }
	    else {
	      Assert (0, "Should not be here");
	    }
	  }
	}
      }
      else {
	if (count == 1) {
	  Expr *tmp;
	  tmp = ret->u.e.l;
	  FREE (ret);
	  ret = tmp;
	}
      }
      act_syn_loop_teardown (ns, s, (char *)e->u.e.l->u.e.l, vx);
    }
    break;
    
  case E_AND:
  case E_OR:
  case E_XOR:
    LVAL_ERROR;
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    ret->u.e.r = _expr_expand (&rw, e->u.e.r, ns, s, flags);
    WIDTH_UPDATE(WIDTH_MAX);
    if (expr_is_a_const (ret->u.e.l) && expr_is_a_const (ret->u.e.r)) {
      if (ret->u.e.l->type == E_INT && ret->u.e.r->type == E_INT) {
	if (ret->u.e.l->u.ival.v_extra && ret->u.e.r->u.ival.v_extra) {
	  BigInt *l = (BigInt *)ret->u.e.l->u.ival.v_extra;
	  BigInt *r = (BigInt *)ret->u.e.r->u.ival.v_extra;

	  if (e->type == E_AND) {
	    *l &= (*r);
	  }
	  else if (e->type == E_OR) {
	    *l |= (*r);
	  }
	  else {
	    *l ^= (*r);
	  }
	  delete r;
	  FREE (ret->u.e.r);
	  FREE (ret->u.e.l);
	  ret->type = E_INT;
	  ret->u.ival.v_extra = l;
	  ret->u.ival.v = l->getVal (0);
	}
	else {
	  unsigned long v;

	  v = ret->u.e.l->u.ival.v;
	  if (e->type == E_AND) {
	    v = v & ((unsigned long)ret->u.e.r->u.ival.v);
	  }
	  else if (e->type == E_OR) {
	    v = v | ((unsigned long)ret->u.e.r->u.ival.v);
	  }
	  else {
	    v = v ^ ((unsigned long)ret->u.e.r->u.ival.v);
	  }
	  //FREE (ret->u.e.l);
	  //FREE (ret->u.e.r);
	  ret->type = E_INT;
	  ret->u.ival.v = v;
	  ret->u.ival.v_extra = NULL;
	  ret = _expr_const_canonical (ret, flags);
	}
      }
      else if (ret->u.e.l->type == E_TRUE || ret->u.e.l->type == E_FALSE) {
	unsigned long v;

	v = (ret->u.e.l->type == E_TRUE) ? 1 : 0;
	if (e->type == E_AND) {
	  v = v & ((ret->u.e.r->type == E_TRUE) ? 1 : 0);
	}
	else if (e->type == E_OR) {
	  v = v | ((ret->u.e.r->type == E_TRUE) ? 1 : 0);
	}
	else {
	  v = v ^ ((ret->u.e.r->type == E_TRUE) ? 1 : 0);
	}
	//FREE (ret->u.e.l);
	//FREE (ret->u.e.r);
	if (v) {
	  ret->type = E_TRUE;
	}
	else {
	  ret->type = E_FALSE;
	}
	ret = _expr_const_canonical (ret, flags);
      }
      else {
	act_error_ctxt (stderr);
	fprintf (stderr, "\texpanding expr: ");
	print_expr (stderr, e);
	fprintf (stderr,"\n");
	fatal_error ("Incompatible types for &/| operator");
      }
    }
    else if ((e->type == E_AND || e->type == E_OR) &&
	     (expr_is_a_const (ret->u.e.l) || expr_is_a_const (ret->u.e.r))) {
      Expr *ce, *re;
      if (expr_is_a_const (ret->u.e.l)) {
	ce = ret->u.e.l;
	re = ret->u.e.r;}
      else {
	ce = ret->u.e.r;
	re = ret->u.e.l;
      }

      if (e->type == E_AND) {
	if (ce->type == E_INT && ce->u.ival.v == 0 &&
	    (!ce->u.ival.v_extra || ((BigInt *)ce->u.ival.v_extra)->isZero())) {
	  if (pc) {
	    /* return 0 */
	    FREE (ret);
	    ret = ce;
	    if (ce->u.ival.v_extra) {
	      ((BigInt *)ce->u.ival.v_extra)->setWidth (*width);
	    }
	    /* XXX: free re but with a new free function */
	  }
	}
	else if (ce->type == E_TRUE) {
	  FREE (ret);
	  ret = re;
	}
	else if (ret->u.e.l->type == E_FALSE) {
	  if (pc) {
	    /* return false */
	    FREE (ret);
	    ret = ce;
	    /* XXX: free re */
	  }
	}
      }
      else if (e->type == E_OR) {
	if (ce->type == E_INT && ce->u.ival.v == 0 &&
	    (!ce->u.ival.v_extra || ((BigInt *)ce->u.ival.v_extra)->isZero())) {
	  FREE (ret);
	  ret = re;
	}
	else if (ce->type == E_TRUE) {
	  if (pc) {
	    /* return true */
	    FREE (ret);
	    ret = ce;
	    /* XXX: free re */
	  }
	}
	else if (ret->u.e.l->type == E_FALSE) {
	  FREE (ret);
	  ret = re;
	}
      }
    }
    break;

  case E_PLUS:
  case E_MINUS:
  case E_MULT:
  case E_DIV: 
  case E_MOD:
  case E_LSL:
  case E_LSR:
  case E_ASR:
    LVAL_ERROR;
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    ret->u.e.r = _expr_expand (&rw, e->u.e.r, ns, s, flags);

    if (e->type == E_PLUS || e->type == E_MINUS) {
      WIDTH_UPDATE (WIDTH_MAX1);
    }
    else if (e->type == E_MULT) {
      WIDTH_UPDATE (WIDTH_SUM);
    }
    else if (e->type == E_DIV || e->type == E_LSR || e->type == E_ASR) {
      WIDTH_UPDATE (WIDTH_LEFT);
    }
    else if (e->type == E_MOD) {
      WIDTH_UPDATE (WIDTH_RIGHT);
    }
    else if (e->type == E_LSL) {
      WIDTH_UPDATE (WIDTH_LSHIFT);
    }
    else {
      Assert (0, "What?");
    }
    
    if (expr_is_a_const (ret->u.e.l) && expr_is_a_const (ret->u.e.r)) {
      if (ret->u.e.l->type == E_INT && ret->u.e.r->type == E_INT) {
	if (ret->u.e.l->u.ival.v_extra && ret->u.e.r->u.ival.v_extra) {
	  BigInt *l = (BigInt *)ret->u.e.l->u.ival.v_extra;
	  BigInt *r = (BigInt *)ret->u.e.r->u.ival.v_extra;

	  if (e->type == E_PLUS) {
	    *l += *r;
	  }
	  else if (e->type == E_MINUS) {
	    *l -= *r;
	  }
	  else if (e->type == E_MULT) {
	    *l = (*l) * (*r);
	  }
	  else if (e->type == E_DIV) {
	    *l = (*l) / (*r);
	  }
	  else if (e->type == E_MOD) {
	    *l = (*l) % (*r);
	  }
	  else if (e->type == E_LSL) {
	    *l <<= (*r);
	  }
	  else if (e->type == E_LSR) {
	    *l >>= (*r);
	  }
	  else {  /* ASR */
	    (*l).toSigned ();
	    *l >>= (*r);
	    (*l).toUnsigned ();
	  }
	  delete r;
	  FREE (ret->u.e.r);
	  FREE (ret->u.e.l);
	  ret->type = E_INT;
	  ret->u.ival.v_extra = l;
	  ret->u.ival.v = l->getVal (0);
	  l->setWidth (*width);
	}
	else {
	  signed long v;

	  v = ret->u.e.l->u.ival.v;
	  if (e->type == E_PLUS) {
	    v = v + ((signed long)ret->u.e.r->u.ival.v);
	  }
	  else if (e->type == E_MINUS) {
	    v = v - ((signed long)ret->u.e.r->u.ival.v);
	  }
	  else if (e->type == E_MULT) {
	    v = v * ((signed long)ret->u.e.r->u.ival.v);
	  }
	  else if (e->type == E_DIV) {
	    v = v / ((signed long)ret->u.e.r->u.ival.v);
	  }
	  else if (e->type == E_MOD) {
	    v = v % ((signed long)ret->u.e.r->u.ival.v);
	  }
	  else if (e->type == E_LSL) {
	    v = v << ((unsigned long)ret->u.e.r->u.ival.v);
	  }
	  else if (e->type == E_LSR) {
	    v = ((unsigned long)v) >> ((unsigned long)ret->u.e.r->u.ival.v);
	  }
	  else { /* ASR */
	    v = (signed long)v >> ((unsigned long)ret->u.e.r->u.ival.v);
	  }
	  //FREE (ret->u.e.l);
	  //FREE (ret->u.e.r);

	  ret->type = E_INT;
	  ret->u.ival.v = v;
	  ret->u.ival.v_extra = NULL;
	  ret = _expr_const_canonical (ret, flags);
	}
#define NUMERIC(e) ((e)->type == E_INT || (e)->type == E_REAL)
#define VAL(e) (((e)->type == E_INT) ? (unsigned long)(e)->u.ival.v : (e)->u.f)
      }
      else if (NUMERIC (ret->u.e.l) && NUMERIC (ret->u.e.r) &&
	       (e->type == E_PLUS || e->type == E_MINUS || e->type == E_MULT
		|| e->type == E_DIV)) {
	double v;

	v = VAL(ret->u.e.l);
	if (e->type == E_PLUS) {
	  v = v + VAL(ret->u.e.r);
	}
	else if (e->type == E_MINUS) {
	  v = v - VAL(ret->u.e.r);
	}
	else if (e->type == E_MULT) {
	  v = v * VAL(ret->u.e.r);
	}
	else if (e->type == E_DIV) {
	  v = v / VAL(ret->u.e.r);
	}
	if (ret->u.e.l->type != E_INT) FREE (ret->u.e.l);
	if (ret->u.e.r->type != E_INT) FREE (ret->u.e.r);
	ret->type = E_REAL;
	ret->u.f = v;
      }
      else {
	act_error_ctxt (stderr);
	fprintf (stderr, "\texpanding expr: ");
	print_expr (stderr, e);
	fprintf (stderr,"\n");
	fatal_error ("Incompatible types for arithmetic operator");
      }
    }
    else if ((e->type == E_PLUS || e->type == E_MINUS || e->type == E_MULT
	      || e->type == E_DIV || e->type == E_MOD) && pc &&
	     (expr_is_a_const (ret->u.e.l) || expr_is_a_const (ret->u.e.r))) {
      Expr *ce, *re;
      if (expr_is_a_const (ret->u.e.l)) {
	ce = ret->u.e.l;
	re = ret->u.e.r;
      }
      else {
	ce = ret->u.e.r;
	re = ret->u.e.l;
      }
      if (e->type == E_PLUS && VAL(ce) == 0) {
	FREE (ret);
	ret = re;
      }
      else if (e->type == E_MINUS && VAL(ce) == 0) {
	if (ce == ret->u.e.r) {
	  FREE (ret);
	  ret = re;
	}
	else {
	  ret->type = E_UMINUS;
	  ret->u.e.l = re;
	  ret->u.e.r = NULL;
	}
      }
      else if (e->type == E_MULT && VAL(ce) == 0) {
	if (pc) {
	  FREE (ret);
	  ret = ce;
	  if (ce->type == E_INT && ce->u.ival.v_extra) {
	    ((BigInt *)ce->u.ival.v_extra)->setWidth (*width);
	  }
	  /* XXX: free re */
	}
      }
      else if (e->type == E_MULT && VAL(ce) == 1) {
	FREE (ret);
	ret = re;
      }
      else if (e->type == E_DIV && VAL(ce) == 0 && (ce == ret->u.e.l)) {
	if (pc) {
	  FREE (ret);
	  ret = ce;
	  if (ce->type == E_INT && ce->u.ival.v_extra) {
	    ((BigInt *)ce->u.ival.v_extra)->setWidth (*width);
	  }
	  /* XXX: free re */
	}
      }
      else if (e->type == E_DIV && VAL(ce) == 1 && (ce == ret->u.e.r)) {
	FREE (ret);
	ret = re;
      }
      else if (e->type == E_MOD && VAL(ce) == 0 && (ce == ret->u.e.l)) {
	if (pc) {
	  FREE (ret);
	  ret = ce;
	  if (ce->type == E_INT && ce->u.ival.v_extra) {
	    ((BigInt *)ce->u.ival.v_extra)->setWidth (*width);
	  }
	  /* XXX: free re */
	}
      }
    }
    break;

  case E_LT:
  case E_GT:
  case E_LE:
  case E_GE:
  case E_EQ:
  case E_NE:
    LVAL_ERROR;
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    ret->u.e.r = _expr_expand (&rw, e->u.e.r, ns, s, flags);
    *width = 1;
    if (expr_is_a_const (ret->u.e.l) && expr_is_a_const (ret->u.e.r)) {
      if (ret->u.e.l->type == E_INT && ret->u.e.r->type == E_INT) {
	if (ret->u.e.l->u.ival.v_extra && ret->u.e.r->u.ival.v_extra) {
	  BigInt *l = (BigInt *)ret->u.e.l->u.ival.v_extra;
	  BigInt *r = (BigInt *)ret->u.e.r->u.ival.v_extra;
	  int res;
	  if (e->type == E_LT) {
	    res = ((*l) < (*r)) ? 1 : 0;
	  }
	  else if (e->type == E_GT) {
	    res = ((*l) > (*r)) ? 1 : 0;
	  }
	  else if (e->type == E_LE) {
	    res = ((*l) <= (*r)) ? 1 : 0;
	  }
	  else if (e->type == E_GE) {
	    res = ((*l) >= (*r)) ? 1 : 0;
	  }
	  else if (e->type == E_EQ) {
	    res = ((*l) == (*r)) ? 1 : 0;
	  }
	  else {
	    res = ((*l) != (*r)) ? 1 : 0;
	  }
	  delete l;
	  delete r;
	  FREE (ret->u.e.l);
	  FREE (ret->u.e.r);
	  if (res) {
	    ret->type = E_TRUE;
	  }
	  else {
	    ret->type = E_FALSE;
	  }
	  ret = _expr_const_canonical (ret, flags);
	}
	else {
	  signed long v;

	  v = ret->u.e.l->u.ival.v;
	  if (e->type == E_LT) {
	    v = (v < ((signed long)ret->u.e.r->u.ival.v) ? 1 : 0);
	  }
	  else if (e->type == E_GT) {
	    v = (v > ((signed long)ret->u.e.r->u.ival.v) ? 1 : 0);
	  }
	  else if (e->type == E_LE) {
	    v = (v <= ((signed long)ret->u.e.r->u.ival.v) ? 1 : 0);
	  }
	  else if (e->type == E_GE) {
	    v = (v >= ((signed long)ret->u.e.r->u.ival.v) ? 1 : 0);
	  }
	  else if (e->type == E_EQ) {
	    v = (v == ((signed long)ret->u.e.r->u.ival.v) ? 1 : 0);
	  }
	  else { /* NE */
	    v = (v != ((signed long)ret->u.e.r->u.ival.v) ? 1 : 0);
	  }
	  //FREE (ret->u.e.l);
	  //FREE (ret->u.e.r);
	  if (v) {
	    ret->type = E_TRUE;
	  }
	  else {
	    ret->type = E_FALSE;
	  }
	  ret = _expr_const_canonical (ret, flags);
	}
      }
      else if (ret->u.e.l->type == E_REAL && ret->u.e.r->type == E_REAL) {
	double v;

	v = ret->u.e.l->u.f;
	if (e->type == E_LT) {
	  v = (v < ret->u.e.r->u.f ? 1 : 0);
	}
	else if (e->type == E_GT) {
	  v = (v > ret->u.e.r->u.f ? 1 : 0);
	}
	else if (e->type == E_LE) {
	  v = (v <= ret->u.e.r->u.f ? 1 : 0);
	}
	else if (e->type == E_GE) {
	  v = (v >= ret->u.e.r->u.f ? 1 : 0);
	}
	else if (e->type == E_EQ) {
	  v = (v == ret->u.e.r->u.f ? 1 : 0);
	}
	else { /* NE */
	  v = (v != ret->u.e.r->u.f ? 1 : 0);
	}
	FREE (ret->u.e.l);
	FREE (ret->u.e.r);
	if (v) {
	  ret->type = E_TRUE;
	}
	else {
	  ret->type = E_FALSE;
	}
	ret = _expr_const_canonical (ret, flags);
      }
      else {
	act_error_ctxt (stderr);
	fprintf (stderr, "\texpanding expr: ");
	print_expr (stderr, e);
	fprintf (stderr,"\n\tlhs: ");
	if (ret->u.e.l->type == E_INT) {
	  fprintf (stderr, "pint");
	}
	else if (ret->u.e.l->type == E_REAL) {
	  fprintf (stderr, "preal");
	}
	else {
	  fprintf (stderr, "other");
	}
	fprintf (stderr,";  rhs: ");
	if (ret->u.e.r->type == E_INT) {
	  fprintf (stderr, "pint");
	}
	else if (ret->u.e.r->type == E_REAL) {
	  fprintf (stderr, "preal");
	}
	else {
	  fprintf (stderr, "other");
	}
	fprintf (stderr, "\n");
	fatal_error ("Incompatible types for comparison operator");
      }
    }
    break;
    
  case E_NOT:
    LVAL_ERROR;
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    *width = lw;
    if (expr_is_a_const (ret->u.e.l)) {
      if (ret->u.e.l->type == E_TRUE) {
	//FREE (ret->u.e.l);
	ret->type = E_FALSE;
	ret = _expr_const_canonical (ret, flags);
      }
      else if (ret->u.e.l->type == E_FALSE) {
	//FREE (ret->u.e.l);
	ret->type = E_TRUE;
	ret = _expr_const_canonical (ret, flags);
      }
      else {
	act_error_ctxt (stderr);
	fprintf (stderr, "\texpanding expr: ");
	print_expr (stderr, e);
	fprintf (stderr,"\n");
	fatal_error ("Incompatible type for not operator");
      }
    }
    break;
    
  case E_COMPLEMENT:
    LVAL_ERROR;
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    *width = lw;
    if (expr_is_a_const (ret->u.e.l)) {
      if (ret->u.e.l->type == E_TRUE) {
	//FREE (ret->u.e.l);
	ret->type = E_FALSE;
	ret = _expr_const_canonical (ret, flags);
      }
      else if (ret->u.e.l->type == E_FALSE) {
	//FREE (ret->u.e.l);
	ret->type = E_TRUE;
	ret = _expr_const_canonical (ret, flags);
      }
      else if (ret->u.e.l->type == E_INT) {
	if (ret->u.e.l->u.ival.v_extra) {
	  BigInt *l = (BigInt *)ret->u.e.l->u.ival.v_extra;
	  *l = ~(*l);
	  FREE (ret->u.e.l);
	  ret->type = E_INT;
	  ret->u.ival.v_extra = l;
	  ret->u.ival.v = l->getVal (0);
	}
	else {
	  unsigned long v = ret->u.e.l->u.ival.v;
	  //FREE (ret->u.e.l);
	  ret->type = E_INT;
	  ret->u.ival.v = ~v;
	  ret->u.ival.v_extra = NULL;
	  ret = _expr_const_canonical (ret, flags);
	}
      }
      else {
	act_error_ctxt (stderr);
	fprintf (stderr, "\texpanding expr: ");
	print_expr (stderr, e);
	fprintf (stderr,"\n");
	fatal_error ("Incompatible type for complement operator");
      }
    }
    break;
    
  case E_UMINUS:
    LVAL_ERROR;
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    *width = lw;
    if (expr_is_a_const (ret->u.e.l)) {
      if (ret->u.e.l->type == E_INT) {
	if (ret->u.e.l->u.ival.v_extra) {
	  BigInt *l = (BigInt *)ret->u.e.l->u.ival.v_extra;
	  l->toSigned ();
	  *l = -(*l);
	  l->toUnsigned ();
	  FREE (ret->u.e.l);
	  ret->type = E_INT;
	  ret->u.ival.v_extra = l;
	  ret->u.ival.v = l->getVal (0);
	}
	else {
	  signed long v = ret->u.e.l->u.ival.v;
	  //FREE (ret->u.e.l);
	  ret->type = E_INT;
	  ret->u.ival.v = -v;
	  ret->u.ival.v_extra = NULL;
	  ret = _expr_const_canonical (ret, flags);
	}
      }
      else if (ret->u.e.l->type == E_REAL) {
	double f = ret->u.e.l->u.f;
	FREE (ret->u.e.l);
	ret->type = E_REAL;
	ret->u.f = -f;
      }
      else {
	act_error_ctxt (stderr);
	fprintf (stderr, "\texpanding expr: ");
	print_expr (stderr, e);
	fprintf (stderr,"\n");
	fatal_error ("Incompatible type for unary minus operator");
      }
    }
    break;

  case E_QUERY:
    LVAL_ERROR;
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    ret->u.e.r = _expr_expand (&rw, e->u.e.r, ns, s, flags);
    WIDTH_UPDATE (WIDTH_MAX); // right, but left width = 1 so this is fine
    if (!expr_is_a_const (ret->u.e.l)) {
      Expr *tmp = ret->u.e.r;
      if (expr_is_a_const (tmp->u.e.l) && expr_is_a_const (tmp->u.e.r)) {
	Expr *ce = tmp->u.e.l;
	if ((tmp->u.e.l->type == E_TRUE && tmp->u.e.r->type == E_TRUE) ||
	    (tmp->u.e.l->type == E_FALSE && tmp->u.e.r->type == E_FALSE) ||
	    (NUMERIC(tmp->u.e.l) && NUMERIC(tmp->u.e.r) &&
	     VAL(tmp->u.e.l) == VAL(tmp->u.e.r))) {
	  /* XXX need to free ret->u.e.l */
	  FREE (ret);
	  FREE (tmp);
	  ret = ce;
	  if (ce->type == E_INT && ce->u.ival.v_extra) {
	    ((BigInt *)ce->u.ival.v_extra)->setWidth (*width);
	  }
	}
      }
    }
    else {
      Expr *tmp = ret->u.e.r;
      if (expr_is_a_const (tmp->u.e.l) && expr_is_a_const (tmp->u.e.r)) {
	//FREE (ret->u.e.l);
	if (ret->u.e.l->type == E_TRUE) {
	  FREE (ret);
	  ret = tmp->u.e.l;
	  expr_ex_free (tmp->u.e.r);
	  FREE (tmp);
	}
	else if (ret->u.e.l->type == E_FALSE) {
	  FREE (ret);
	  ret = tmp->u.e.r;
	  expr_ex_free (tmp->u.e.l);
	  FREE (tmp);
	}
	else {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr,"\n");
	  fatal_error ("Query operator expression has non-Boolean value");
	}
	if (ret->type == E_INT && ret->u.ival.v_extra) {
	  ((BigInt *)ret->u.ival.v_extra)->setWidth (*width);
	}
      }
    }
    break;

  case E_COLON:
    LVAL_ERROR;
    /* you only get here for non-const things */
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    ret->u.e.r = _expr_expand (&rw, e->u.e.r, ns, s, flags);
    WIDTH_UPDATE (WIDTH_MAX);
    break;

  case E_BITFIELD:
    LVAL_ERROR;
    if (flags & ACT_EXPR_EXFLAG_DUPONLY) {
      ret->u.e.l = (Expr *) ((ActId *)e->u.e.l)->Clone();
      NEW (ret->u.e.r, Expr);
      ret->u.e.r->type = E_BITFIELD;
      ret->u.e.r->u.e.l = _expr_expand (&lw, e->u.e.r->u.e.l, ns, s, flags);
      ret->u.e.r->u.e.r = _expr_expand (&rw, e->u.e.r->u.e.r, ns, s, flags);
      *width = -1;
    }
    else {
      if (flags & ACT_EXPR_EXFLAG_CHPEX) {
	ActId *xid = ((ActId *)e->u.e.l)->ExpandCHP (ns, s);
	ret->u.e.l = (Expr *) xid;
	te = xid->EvalCHP (ns, s, 0);
	if (!expr_is_a_const (te)) {
	  if (te->type != E_VAR) {
	    expr_ex_free (te);
	  }
	  else {
	    FREE (te);
	  }
	}
	else {
	  delete xid;
	  ret->u.e.l = te;
	}
      }
      else {
	ret->u.e.l = (Expr *) ((ActId *)e->u.e.l)->Expand (ns, s);
      }
      if (!expr_is_a_const (ret->u.e.l)) {
	NEW (ret->u.e.r, Expr);
	ret->u.e.r->type = E_BITFIELD;
	ret->u.e.r->u.e.l = _expr_expand (&lw, e->u.e.r->u.e.l, ns, s, flags);
	ret->u.e.r->u.e.r = _expr_expand (&rw, e->u.e.r->u.e.r, ns, s, flags);
	if ((ret->u.e.r->u.e.l && !expr_is_a_const (ret->u.e.r->u.e.l)) || !expr_is_a_const (ret->u.e.r->u.e.r)) {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr,"\n");
	  fatal_error ("Bitfield operator has non-const components");
	}
	if (ret->u.e.r->u.e.l && (ret->u.e.r->u.e.l->type != E_INT)) {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr,"\n");
	  fatal_error ("Variable in bitfield operator is a non-integer");
	}
	if (ret->u.e.r->u.e.r->type != E_INT) {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr,"\n");
	  fatal_error ("Variable in bitfield operator is a non-integer");
	}
	if (ret->u.e.r->u.e.l) {
	  *width = ret->u.e.r->u.e.r->u.ival.v - ret->u.e.r->u.e.l->u.ival.v + 1;
	}
	else {
	  *width = 1;
	}
	InstType *it = s->FullLookup ((ActId *)ret->u.e.l, NULL);
	if (ret->u.e.r->u.e.r->u.ival.v >= TypeFactory::bitWidth (it)) {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr, "\n");
	  warning ("Bit-width (%d) is less than the width specifier {%d..%d}",
		   TypeFactory::bitWidth (it), ret->u.e.r->u.e.r->u.ival.v,
		   ret->u.e.r->u.e.l ? ret->u.e.r->u.e.l->u.ival.v :
		   ret->u.e.r->u.e.r->u.ival.v);
	}
      }
      else {
	unsigned long v;
	Expr *lo, *hi;

	if (ret->u.e.l->type != E_INT) {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr,"\n");
	  fatal_error ("Variable in bitfield operator is a non-integer");
	}
	v = ret->u.e.l->u.ival.v;
	if (e->u.e.r->u.e.l) {
	  lo = _expr_expand (&lw, e->u.e.r->u.e.l, ns, s, flags);
	}
	else {
	  lo = NULL;
	  lw = -1;
	}
	hi = _expr_expand (&rw, e->u.e.r->u.e.r, ns, s, flags);
	Assert (hi, "What?");
      
	unsigned long lov, hiv;

	if ((lo && lo->type != E_INT) || hi->type != E_INT) {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr,"\n");
	  fatal_error ("Bitfield parameter in operator is a non-integer");
	}

	hiv = hi->u.ival.v;
	if (lo) {
	  lov = lo->u.ival.v;
	}
	else {
	  lov = hiv;
	}
      
	if (lov > hiv) {
	  act_error_ctxt (stderr);
	  fprintf (stderr, "\texpanding expr: ");
	  print_expr (stderr, e);
	  fprintf (stderr, "\n");
	  fatal_error ("Bitfield {%d..%d} : empty!", hiv, lov);
	}

	v = (v >> lov) & ~(~0UL << (hiv - lov + 1));

	BigInt *ltmp;
	if (ret->u.e.l->u.ival.v_extra) {
	  ltmp = (BigInt *) ret->u.e.l->u.ival.v_extra;
	}
	else {
	  ltmp = NULL;
	}

	ret->type = E_INT;
	ret->u.ival.v = v;

	if (ltmp) {
	  *ltmp >>= lov;
	  FREE (ret->u.e.l);
	  ltmp->setWidth (hiv-lov+1);
	  ret->u.ival.v_extra = ltmp;
	  ret->u.ival.v = ltmp->getVal (0);
	}
	else {
	  ret->u.ival.v_extra = NULL;
	  ret = _expr_const_canonical (ret, flags);
	}
	*width = (hiv - lov + 1);
      }
    }
    break;

  case E_PROBE:
    LVAL_ERROR;
    if (flags & ACT_EXPR_EXFLAG_DUPONLY) {
      ret->u.e.l = (Expr *) ((ActId *)e->u.e.l)->Clone ();
    }
    else {
      ActId *tmp = ((ActId *)e->u.e.l)->ExpandCHP (ns, s);
      Expr *te = tmp->EvalCHP (ns, s, 0);
      Assert (te->type == E_VAR, "Whaat? Probe turned into something else?");
      if (tmp->isDynamicDeref()) {
	act_error_ctxt (stderr);
	fprintf (stderr, "Dynamic channel array probes are unsupported.\n");
	fprintf (stderr, "\tVariable: ");
	tmp->Print (stderr);
	fprintf (stderr, "\n");
	exit (1);
      }
      FREE (ret);
      ret = te;
      ret->type = E_PROBE;
    }
    *width = 1;
    break;

  case E_BUILTIN_INT:
  case E_BUILTIN_BOOL:
    LVAL_ERROR;
    if (flags & ACT_EXPR_EXFLAG_PREEXDUP) {
      ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
      ret->u.e.r = _expr_expand (&rw, e->u.e.r, ns, s, flags);
      *width = -1;
    }
    else {
      ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
      if (!e->u.e.r) {
	ret->u.e.r = NULL;
	if (expr_is_a_const (ret->u.e.l)) {
	  if (ret->type == E_BUILTIN_BOOL) {
	    if (ret->u.e.l->u.ival.v) {
	      ret->type = E_TRUE;
	      ret = _expr_const_canonical (ret, flags);
	    }
	    else {
	      ret->type = E_FALSE;
	      ret = _expr_const_canonical (ret, flags);
	    }
	  }
	  else {
	    if (ret->u.e.l->type == E_TRUE) {
	      ret->type = E_INT;
	      ret->u.ival.v = 1;
	      ret->u.ival.v_extra = NULL;
	      ret = _expr_const_canonical (ret, flags);
	    }
	    else if (ret->u.e.l->type == E_REAL) {
	      Expr *texpr = ret->u.e.l;
	      ret->type = E_INT;
	      ret->u.ival.v = (long)texpr->u.f;
	      ret->u.ival.v_extra = NULL;
	      ret = _expr_const_canonical (ret, flags);
	      expr_free (texpr);
	    }
	    else {
	      ret->type = E_INT;
	      ret->u.ival.v = 0;
	      ret->u.ival.v_extra = NULL;
	      ret = _expr_const_canonical (ret, flags);
	    }
	  }
	}
	*width = 1;
      }
      else {
	ret->u.e.r = _expr_expand (&lw, e->u.e.r, ns, s, flags);
	if (expr_is_a_const (ret->u.e.l) && expr_is_a_const (ret->u.e.r)) {
	  BigInt *l;
	  int _width = ret->u.e.r->u.ival.v;

	  if (ret->u.e.l->u.ival.v_extra) {
	    l = (BigInt *)ret->u.e.l->u.ival.v_extra;
	    FREE (ret->u.e.l);
	    ret->type = E_INT;
	    l->setWidth (_width);
	    ret->u.ival.v_extra = l;
	    ret->u.ival.v = l->getVal (0);
	  }
	  else {
	    unsigned long x = ret->u.e.l->u.ival.v;

	    if (_width < 64) {
	      x = x & ((1UL << _width)-1);
	    }

	    l = new BigInt;
	    l->setWidth (_width);
	    l->setVal (0, x);
	  
	    ret->type = E_INT;
	    ret->u.ival.v = x;
	    ret->u.ival.v_extra = l;
	  }
	  *width = _width;
	}
	else if (!expr_is_a_const (ret->u.e.r)) {
	  act_error_ctxt (stderr);
	  fatal_error ("int() operator requires a constant expression for the second argument");
	}
	else {
	  *width = ret->u.e.r->u.ival.v;
	}
      }
    }
    break;
    
  case E_FUNCTION:
    LVAL_ERROR;
    if (!(flags & ACT_EXPR_EXFLAG_CHPEX)) {
      /* non-chp mode: this must be a parameter thing */
      if (flags & ACT_EXPR_EXFLAG_DUPONLY) {
	Expr *tmp, *prev;
	Expr *w = e->u.fn.r;
	ret->u.fn.s = e->u.fn.s;
	ret->u.fn.r = NULL;
	prev = NULL;
	while (w) {
	  int dummy;
	  NEW (tmp, Expr);
	  tmp->type = w->type;
	  tmp->u.e.l = _expr_expand (&dummy, w->u.e.l, ns, s, flags);
	  tmp->u.e.r = NULL;
	  if (!prev) {
	    ret->u.fn.r = tmp;
	  }
	  else {
	    prev->u.e.r = tmp;
	  }
	  prev = tmp;
	  w = w->u.e.r;
	}
      }
      else {
	_eval_function (ns, s, e, &ret, flags);
      }
      *width = -1;
    }
    else {
      Expr *tmp, *etmp;
      Function *f = dynamic_cast<Function *>((UserDef *)e->u.fn.s);
      if (TypeFactory::isParamType (f->getRetType())) {
	// parameterized function: we need to evaluate this
	// statically, even in CHP mode
	_eval_function (ns, s, e, &ret, flags & ~ACT_EXPR_EXFLAG_CHPEX);
	*width = -1;
      }
      else {
	*width = TypeFactory::bitWidth (f->getRetType());
      
	if (e->u.fn.r && e->u.fn.r->type == E_GT) {
	  /* template parameters */
	  int count = 0;
	  Expr *w;
	  w = e->u.fn.r->u.e.l;
	  while (w) {
	    count++;
	    w = w->u.e.r;
	  }
	  inst_param *inst;
	  if (count == 0) {
	    if (!f->isExpanded()) {
	      f = f->Expand (ns, s, 0, NULL);
	    }
	  }
	  else {
	    MALLOC (inst, inst_param, count);
	    w = e->u.fn.r->u.e.l;
	    for (int i=0; i < count; i++) {
	      AExpr *tae;
	      inst[i].isatype = 0;
	      tae = new AExpr (expr_dup (w->u.e.l));
	      inst[i].u.tp = tae->Expand (ns, s, 0);
	      delete tae;
	      w = w->u.e.r;
	    }
	    f = f->Expand (ns, s, count, inst);
	    for (int i=0; i < count; i++) {
	      delete inst[i].u.tp;
	    }
	    FREE (inst);
	  }
	}
	else {
	  if (flags & ACT_EXPR_EXFLAG_DUPONLY) {
	    /* nothing here */
	  }
	  else {
	    if (!f->isExpanded()) {
	      f = f->Expand (ns, s, 0, NULL);
	    }
	  }
	}

	if (f->isExternal()) {
	  _act_chp_is_synth_flag = 0;
	}
      
	ret->u.fn.s = (char *) f;
	if (!e->u.fn.r) {
	  ret->u.fn.r = NULL;
	}
	else {
	  NEW (tmp, Expr);
	  tmp->type = E_LT;
	  ret->u.fn.r = tmp;
	  tmp->u.e.r = NULL;
	  if (e->u.fn.r->type == E_GT) {
	    etmp = e->u.fn.r->u.e.r;
	  }
	  else {
	    etmp = e->u.fn.r;
	  }
	  do {
	    tmp->u.e.l = _expr_expand (&lw, etmp->u.e.l, ns, s, flags);
	    if (etmp->u.e.r) {
	      NEW (tmp->u.e.r, Expr);
	      tmp->u.e.r->type = E_LT;
	      tmp = tmp->u.e.r;
	      tmp->u.e.r = NULL;
	    }
	    etmp = etmp->u.e.r;
	  } while (etmp);
	}
      }
    }
    if (e_orig) {
      /* usermacro substitution; so just free the tmp Expr that was
	 allocated. */
      FREE (e);
      e = e_orig;
    }
    break;

  case E_VAR:
    /* expand an ID:
       this either returns an expanded ID, or 
       for parameterized types returns the value. */
    if (flags & ACT_EXPR_EXFLAG_DUPONLY) {
      ret->u.e.l = (Expr *)((ActId *)e->u.e.l)->Clone ();
      *width = -1;
    }
    else {
      if (flags & ACT_EXPR_EXFLAG_CHPEX) {
	/* chp mode expansion */
	xid = ((ActId *)e->u.e.l)->ExpandCHP (ns, s);
	te = xid->EvalCHP (ns, s, 0);
	if (te->type == E_VAR) {
	  act_chp_macro_check (s, (ActId *)te->u.e.l);
	  InstType *it;
	  act_type_var (s, (ActId *)te->u.e.l, &it);
	  *width = TypeFactory::bitWidth (it);
	}
	else if (te->type == E_INT) {
	  if (te->u.ival.v_extra == NULL) {
	    /* XXX: add stuff here */
	  }
	  *width = act_expr_intwidth (te->u.ival.v);
	}
	else {
	  *width = -1;
	}
      }
      else {
	/* non-chp expansion */
	xid = ((ActId *)e->u.e.l)->Expand (ns, s);
	te = xid->Eval (ns, s, (flags & ACT_EXPR_EXFLAG_ISLVAL) ? 1 : 0);
	if (te->type == E_INT) {
	  *width = act_expr_intwidth (te->u.ival.v);
	}
	else {
	  *width = -1;
	}
      }
      if (te->type != E_VAR) {
	delete xid;
      }
      FREE (ret);
      ret = te;
    }
    break;

  case E_INT:
    LVAL_ERROR;
    ret->u.ival.v = e->u.ival.v;
    ret->u.ival.v_extra = NULL;
    *width = act_expr_intwidth (ret->u.ival.v);

    if (flags & ACT_EXPR_EXFLAG_CHPEX) {
      if (e->u.ival.v_extra) {
	BigInt *btmp = new BigInt();
	*btmp = *((BigInt *)e->u.ival.v_extra);
	ret->u.ival.v_extra = btmp;
	*width = btmp->getWidth ();
      }
      else {
	BigInt *btmp = new BigInt (*width, 0, 1);
	btmp->setVal (0, ret->u.ival.v);
	ret->u.ival.v_extra = btmp;
      }
    }
    else {
      if (e->u.ival.v_extra) {
	BigInt *btmp = new BigInt();
	*btmp = *((BigInt *)e->u.ival.v_extra);
	ret->u.ival.v_extra = btmp;
	*width = btmp->getWidth();
      }
      else {
	ret = _expr_const_canonical (ret, flags);
      }
    }
    break;

  case E_REAL:
    LVAL_ERROR;
    ret->u.f = e->u.f;
    *width = 64;
    break;

  case E_TRUE:
  case E_FALSE:
    LVAL_ERROR;
    ret = _expr_const_canonical (ret, flags);
    *width = 1;
    break;

  case E_ARRAY:
  case E_SUBRANGE:
    ret->u = e->u;
    break;

  case E_SELF:
    xid = new ActId ("self");
    te = xid->Eval (ns, s, (flags & ACT_EXPR_EXFLAG_ISLVAL) ? 1 : 0);
    if (te->type != E_VAR) {
      delete xid;
    }
    FREE (ret);
    ret = te;
    *width = 0;
    break;

  case E_SELF_ACK:
    xid = new ActId ("selfack");
    te = xid->Eval (ns, s, (flags & ACT_EXPR_EXFLAG_ISLVAL) ? 1 : 0);
    if (te->type != E_VAR) {
      delete xid;
    }
    FREE (ret);
    ret = te;
    *width = 0;
    break;

  case E_CONCAT:
    {
      Expr *f = ret;
      ret->u.e.l = NULL;
      ret->u.e.r = NULL;
      *width = 0;
      while (e) {
	f->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s,
				(flags & ~ACT_EXPR_EXFLAG_ISLVAL));
	*width += lw;
	if (e->u.e.r) {
	  NEW (f->u.e.r, Expr);
	  f = f->u.e.r;
	  f->type = E_CONCAT;
	  f->u.e.l = NULL;
	  f->u.e.r = NULL;
	}
	e = e->u.e.r;
      }
    }
    break;

  case E_ENUM_CONST:
    {
      Data *d = (Data *) e->u.fn.s;
      d = d->Expand (d->getns(), d->getns()->CurScope(), 0, NULL);
      ret->type = E_INT;
      ret->u.ival.v = d->enumVal ((char *)e->u.fn.r);
      ret->u.ival.v_extra = NULL;
      *width = TypeFactory::bitWidth (d);
      if (flags & ACT_EXPR_EXFLAG_CHPEX) {
	BigInt *btmp = new BigInt (*width, 0, 1);
	btmp->setVal (0, ret->u.ival.v);
	ret->u.ival.v_extra = btmp;
      }
      else {
	ret = _expr_const_canonical (ret, flags);
      }
    }
    break;

  case E_STRUCT_REF:
    ret->u.e.l = _expr_expand (&lw, e->u.e.l, ns, s, flags);
    {
      InstType *it = act_expr_insttype (s, ret->u.e.l, NULL, 2);
      Assert (it, "What?");
      Assert (TypeFactory::isPureStruct (it), "What");
      UserDef *ux = dynamic_cast<UserDef *> (it->BaseType());
      Assert (ux, "What");

      ret->u.e.r = _expr_expand (&lw, e->u.e.r, ns, ux->CurScope(), flags);
      if (expr_is_a_const (ret->u.e.r)) {
	Expr *tmp = ret;
	ret = ret->u.e.r;
	expr_ex_free (tmp->u.e.l);
	FREE (tmp);
      }
    }
    break;
    
  default:
    fatal_error ("Unknown expression type (%d)!", e->type);
    break;
  }
  if (ret->type == E_INT && ret->u.ival.v_extra) {
    BigInt *tmp = (BigInt *) ret->u.ival.v_extra;
    Assert (tmp->getWidth () == *width, "What?");
  }
  return ret;
}

Expr *expr_expand (Expr *e, ActNamespace *ns, Scope *s, unsigned int flags)
{
  int w;
  return _expr_expand (&w, e, ns, s, flags);
}

/*------------------------------------------------------------------------
 *  Is this a constant? This works only after it has been expanded out.
 *------------------------------------------------------------------------
 */
int expr_is_a_const (Expr *e)
{
  Assert (e, "What?");
  if (e->type == E_INT || e->type == E_REAL ||
      e->type == E_TRUE || e->type == E_FALSE) {
    return 1;
  }
  
  return 0;
}

Expr *expr_dup_const (Expr *e)
{
  Assert (expr_is_a_const (e), "What?");
  if (e->type != E_REAL) {
    return e;
  }
  else {
    Expr *ret;
    NEW (ret, Expr);
    *ret = *e;
    return ret;
  }
}


/*
 *  Construct a hash table that specifies the bit-widths of all the
 *  parts of the expression.
 *
 */
static int _expr_bw_calc(struct pHashtable *H, Expr *e, Scope *s)
{
  int lw, rw, width;
  Expr *l, *r;
  phash_bucket_t *pb;

  if (!e) return 0;

  pb = phash_lookup (H, e);

  if (pb) {
    return pb->i;
  }

  switch (e->type) {
  case E_ANDLOOP:
  case E_ORLOOP:
  case E_PLUSLOOP:
  case E_MULTLOOP:
  case E_XORLOOP:
  case E_ARRAY:
  case E_SUBRANGE:
  case E_ENUM_CONST:
    Assert (0, "Should not be here!\n");
    break;
    
  case E_AND:
  case E_OR:
  case E_XOR:
  case E_PLUS:
  case E_MINUS:
  case E_MULT:
  case E_DIV: 
  case E_MOD:
  case E_LSL:
  case E_LSR:
  case E_ASR:
  case E_LT:
  case E_GT:
  case E_LE:
  case E_GE:
  case E_EQ:
  case E_NE:
    lw = _expr_bw_calc (H, e->u.e.l, s);
    rw = _expr_bw_calc (H, e->u.e.r, s);
    width = act_expr_bitwidth (e->type, lw, rw);
    break;
    
  case E_NOT:
  case E_COMPLEMENT:
  case E_UMINUS:
    lw = _expr_bw_calc (H, e->u.e.l, s);
    width = act_expr_bitwidth (e->type, lw, lw);
    break;
    
  case E_QUERY:
    lw = _expr_bw_calc (H, e->u.e.l, s);
    lw = _expr_bw_calc (H, e->u.e.r->u.e.l, s);
    rw = _expr_bw_calc (H, e->u.e.r->u.e.r, s);
    width = act_expr_bitwidth (e->type, lw, rw);
    break;

  case E_COLON:
    Assert (0, "Should not be here / E_COLON!");
    break;

  case E_BITFIELD:
    if (e->u.e.r->u.e.l) {
      Assert (e->u.e.r->u.e.l->type == E_INT, "What?");
      Assert (e->u.e.r->u.e.r->type == E_INT, "What?");
      width = (e->u.e.r->u.e.r->u.ival.v - e->u.e.r->u.e.l->u.ival.v) + 1;
    }
    else {
      width = 1;
    }
    break;

  case E_PROBE:
    width = 1;
    break;

  case E_BUILTIN_INT:
    lw = _expr_bw_calc (H, e->u.e.l, s);
    if (e->u.e.r) {
      Assert (e->u.e.r->type == E_INT, "What?");
      width = e->u.e.r->u.ival.v;
    }
    else {
      width = 1;
    }
    break;
    
  case E_BUILTIN_BOOL:
    lw = _expr_bw_calc (H, e->u.e.l, s);
    width = 1;
    break;
    
  case E_FUNCTION:
    {
      // process arguments
      Expr *tmp = e->u.fn.r;
      if (tmp) {
	Assert (tmp->type != E_GT, "Template params still here?");
      }
      while (tmp) {
	lw = _expr_bw_calc (H, tmp->u.e.l, s);
	tmp = tmp->u.e.r;
      }
      // width from the return type of the function
      UserDef *u = (UserDef *) e->u.fn.s;
      Assert (TypeFactory::isFuncType (u), "What?");
      Function *fn = dynamic_cast<Function *> (u);
      width = TypeFactory::bitWidth (fn->getRetType());
    }
    break;

  case E_VAR:
    {
      InstType *xit;
      lw = act_type_var (s, (ActId *)e->u.e.l, &xit);
      width = TypeFactory::bitWidth (xit);
    }
    break;

  case E_INT:
    if (e->u.ival.v_extra) {
      width = ((BigInt *)e->u.ival.v_extra)->getWidth ();
    }
    else {
      width = act_expr_intwidth (e->u.ival.v);
    }
    break;

  case E_REAL:
    Assert (0, "E_REAL in CHP/dataflow expr?");
    break;

  case E_TRUE:
  case E_FALSE:
    width = 1;
    break;

  case E_SELF:
  case E_SELF_ACK:
    {
      ActId *tmpid;
      InstType *xit;
      if (e->type == E_SELF) {
	tmpid = new ActId ("self");
      }
      else {
	tmpid = new ActId ("selfack");
      }
      lw =  act_type_var (s, tmpid, &xit);
      width = TypeFactory::bitWidth (xit);
      delete tmpid;
    }
    break;

  case E_CONCAT:
    {
      Expr *tmp = e;
      width = 0;
      do {
	lw = _expr_bw_calc (H, tmp->u.e.l, s);
	width += lw;
	tmp = tmp->u.e.r;
      } while (tmp);
    }
    break;

  case E_STRUCT_REF:
    Assert (0, "This should be called after chp_unstruct()");
    break;
    
  default:
    fatal_error ("Unknown expression type (%d)!", e->type);
    break;
  }
  pb = phash_add (H, e);
  pb->i = width;
  return width;
}



/*
 * This takes an expanded CHP/dataflow expression.
 *
 *   needed_width : the # of bits of result that are actually needed
 *                  set this to -1 if all bits are needed
 *
 *
 * The net effect of this is to add/update int() calls into the
 * expression to throw out bits that are not needed as early as possible.
 *
 */
static int _getbw (struct pHashtable *H, Expr *e)
{
  phash_bucket_t *pb;
  pb = phash_lookup (H, e);
  Assert (pb, "What happened?");
  return pb->i;
}

static Expr *_wrapint (Expr *e, int w)
{
  Expr *ret;
  NEW (ret, Expr);
  ret->type = E_BUILTIN_INT;
  ret->u.e.l = e;
  ret->u.e.r = const_expr (w);
  return ret;
}

static Expr *_wrapbool2int (Expr *e)
{
  Expr *ret;
  NEW (ret, Expr);
  ret->type = E_BUILTIN_INT;
  ret->u.e.l = e;
  ret->u.e.r = NULL;
  return ret;
}


static Expr *_expr_bw_adjust (struct pHashtable *H,
			      int needed_width, Expr *e, Scope *s);

static void _expr_cmp_helper (Expr *ret, Expr *l, Expr *r,
			       int lw, int rw,
			       int boolone, int booltwo,
			       struct pHashtable *H,
			       Scope *s)
{
  int type = ret->type;

  if (lw == rw) {
    ret->u.e.l = _expr_bw_adjust (H, -1, l, s);
    ret->u.e.r = _expr_bw_adjust (H, -1, r, s);
    return;
  }

  ret->type = E_QUERY;
  NEW (ret->u.e.l, Expr);
  ret->u.e.l->type = E_EQ;
  ret->u.e.l->u.e.r = const_int_ex (0);
  NEW (ret->u.e.l->u.e.l, Expr);  
  ret->u.e.l->u.e.l->type = E_LSR;

  NEW (ret->u.e.r, Expr);
  ret->u.e.r->type = E_COLON;

  NEW (ret->u.e.r->u.e.l, Expr);
  ret->u.e.r->u.e.l->type = type;
  
  if (lw < rw) {
    ret->u.e.l->u.e.l->u.e.r = const_int_ex (lw);
    ret->u.e.l->u.e.l->u.e.l = _expr_bw_adjust (H, -1, r, s);
    
    ret->u.e.r->u.e.r = const_int_ex (boolone);
    
    ret->u.e.r->u.e.l->u.e.l = _expr_bw_adjust (H, -1, l, s);
    ret->u.e.r->u.e.l->u.e.r = _expr_bw_adjust (H, lw, r, s);
  }
  else {
    ret->u.e.l->u.e.l->u.e.r = const_int_ex (rw);
    ret->u.e.l->u.e.l->u.e.l = _expr_bw_adjust (H, -1, l, s);
    
    ret->u.e.r->u.e.r = const_int_ex (booltwo);
    
    ret->u.e.r->u.e.l->u.e.l = _expr_bw_adjust (H, rw, l, s);
    ret->u.e.r->u.e.l->u.e.r = _expr_bw_adjust (H, -1, r, s);
  }

  ret->u.e.r->u.e.l = _wrapbool2int (ret->u.e.r->u.e.l);
  Expr *tmp;
  NEW (tmp, Expr);
  *tmp = *ret;
  ret->type = E_BUILTIN_BOOL;
  ret->u.e.l = tmp;
  ret->u.e.r = NULL;
}

static Expr *_expr_bw_adjust (struct pHashtable *H,
			      int needed_width, Expr *e, Scope *s)
{
  int lw, rw;
  Expr *ret = NULL;
  Expr *l, *r;

  if (!e) return NULL;

  NEW (ret, Expr);
  ret->type = e->type;
  ret->u.e.l = NULL;
  ret->u.e.r = NULL;

  switch (e->type) {
  case E_ANDLOOP:
  case E_ORLOOP:
  case E_PLUSLOOP:
  case E_MULTLOOP:
  case E_XORLOOP:
  case E_ARRAY:
  case E_SUBRANGE:
  case E_ENUM_CONST:
    Assert (0, "Should not be here!\n");
    break;
    
  case E_AND:
  case E_OR:
  case E_XOR:
    l = _expr_bw_adjust (H, needed_width, e->u.e.l, s);
    r = _expr_bw_adjust (H, needed_width, e->u.e.r, s);
    ret->u.e.l = l;
    ret->u.e.r = r;
    break;

  case E_PLUS:
  case E_MINUS:
  case E_MULT:
    l = _expr_bw_adjust (H, needed_width, e->u.e.l, s);
    r = _expr_bw_adjust (H, needed_width, e->u.e.r, s);
    ret->u.e.l = l;
    ret->u.e.r = r;
    if (needed_width != -1 && _getbw (H, e) > needed_width) {
      ret = _wrapint (ret, needed_width);
    }
    break;
    
  case E_LSL:
    if (needed_width == -1) {
      l = _expr_bw_adjust (H, needed_width, e->u.e.l, s);
      r = _expr_bw_adjust (H, needed_width, e->u.e.r, s);
      ret->u.e.l = l;
      ret->u.e.r = r;
    }
    else {
      lw = act_expr_intwidth (needed_width);
      // XXX: here new expression
      ret->type = E_QUERY;
      NEW (ret->u.e.r, Expr);
      ret->u.e.r->type = E_COLON;

      // ((b) >> lgw) != 0
      NEW (ret->u.e.l, Expr);
      ret->u.e.l->type = E_NE;
      ret->u.e.l->u.e.r = const_int_ex (0);

      // (b) >> lgw
      NEW (ret->u.e.l->u.e.l, Expr);
      ret->u.e.l->u.e.l->type = E_LSR;
      ret->u.e.l->u.e.l->u.e.l = _expr_bw_adjust (H, -1, e->u.e.r, s);
      ret->u.e.l->u.e.l->u.e.r = const_int_ex (lw);

      ret->u.e.r->u.e.l = _wrapint (const_int_ex (0), needed_width);

      Expr *tmp;
      NEW (tmp, Expr);
      tmp->type = E_LSL;
      tmp->u.e.l = _expr_bw_adjust (H, -1, e->u.e.l, s);
      tmp->u.e.r = _wrapint (_expr_bw_adjust (H, -1, e->u.e.r,s), lw);
      ret->u.e.r->u.e.r = _wrapint (tmp, needed_width);
    }
    break;
    
  case E_LSR:
  case E_ASR:
  case E_DIV:
  case E_MOD:
    l = _expr_bw_adjust (H, -1, e->u.e.l, s);
    r = _expr_bw_adjust (H, -1, e->u.e.r, s);
    ret->u.e.l = l;
    ret->u.e.r = r;
    if (needed_width != -1 && _getbw (H, e) > needed_width) {
      ret = _wrapint (ret, needed_width);
    }
    break;
    
  case E_GT:
  case E_GE:
    lw = _getbw (H, e->u.e.l);
    rw = _getbw (H, e->u.e.r);
    _expr_cmp_helper (ret, e->u.e.l, e->u.e.r, lw, rw, 0, 1, H, s);
    break;
    
  case E_LE:
  case E_LT:
    lw = _getbw (H, e->u.e.l);
    rw = _getbw (H, e->u.e.r);
    _expr_cmp_helper (ret, e->u.e.l, e->u.e.r, lw, rw, 1, 0, H, s);
    break;
    
  case E_EQ:
    lw = _getbw (H, e->u.e.l);
    rw = _getbw (H, e->u.e.r);
    _expr_cmp_helper (ret, e->u.e.l, e->u.e.r, lw, rw, 0, 0, H, s);
    break;
    
  case E_NE:
    lw = _getbw (H, e->u.e.l);
    rw = _getbw (H, e->u.e.r);
    _expr_cmp_helper (ret, e->u.e.l, e->u.e.r, lw, rw, 1, 1, H, s);
    break;
    
  case E_NOT:
  case E_COMPLEMENT:
  case E_UMINUS:
    /* width matches, nothing left to do */
    ret->u.e.l = _expr_bw_adjust (H, needed_width, e->u.e.l, s);
    ret->u.e.r = NULL;
    break;
    
  case E_QUERY:
    ret->u.e.l = _expr_bw_adjust (H, 1, e->u.e.l, s);
    NEW (ret->u.e.r, Expr);
    ret->u.e.r->type = E_COLON;
    ret->u.e.r->u.e.l = _expr_bw_adjust (H, needed_width, e->u.e.r->u.e.l, s);
    ret->u.e.r->u.e.r = _expr_bw_adjust (H, needed_width, e->u.e.r->u.e.r, s);
    break;

  case E_COLON:
    Assert (0, "E_COLON?!");
    break;

  case E_BITFIELD:
    FREE (ret);
    if (needed_width == -1) {
      ret = expr_dup (e);
    }
    else {
      lw = _getbw (H, e);
      if (lw > needed_width) {
	ret = _wrapint (expr_dup (e), needed_width);
      }
      else {
	ret = expr_dup (e);
      }
    }
    break;

  case E_PROBE:
    FREE (ret);
    ret = expr_dup (e);
    break;

  case E_BUILTIN_INT:
    if (e->u.e.r) {
      int mw;
      if (needed_width == -1 || (e->u.e.r->u.ival.v <= needed_width)) {
	mw = e->u.e.r->u.ival.v;
      }
      else {
	mw = needed_width;
      }
      l = _expr_bw_adjust (H, mw, e->u.e.l, s);
    }
    else {
      // bool to int conv
      l = _expr_bw_adjust (H, 1, e->u.e.l, s);
    }
    ret->u.e.l = l;
    if (needed_width == -1 /* don't fiddle with widths */
	|| !e->u.e.r /* bitwidth is 1 */
	|| needed_width >= (e->u.e.r->u.ival.v)) {
      ret->u.e.r = expr_dup (e->u.e.r);
    }
    else {
      ret->u.e.r = const_expr (needed_width);
    }
    break;
    
  case E_BUILTIN_BOOL:
    ret->u.e.l = _expr_bw_adjust (H, needed_width, e->u.e.l, s);
    ret->u.e.r = NULL;
    break;
    
  case E_FUNCTION:
    // walk through the arguments; the needed bitwidth is specified by
    // the argument type!
    ret->u.fn.s = e->u.fn.s;
    ret->u.fn.r = NULL;
    {
      int i = 0;
      UserDef *u = (UserDef *) e->u.fn.s;
      Function *fn = dynamic_cast<Function *> (u);
      Assert (fn, "What?");
      Expr *args = e->u.fn.r;
      Expr *tmp = NULL;
      while (args) {
	if (!tmp) {
	  NEW (ret->u.fn.r, Expr);
	  Assert (e->u.fn.r->type == E_LT, "Hm");
	  tmp = ret->u.fn.r;
	}
	else {
	  NEW (tmp->u.e.r, Expr);
	  tmp = tmp->u.e.r;
	}
	tmp->type = E_LT;
	tmp->u.e.r = NULL;
	Assert (i < fn->getNumPorts(), "What?");
	if (TypeFactory::isStructure (fn->getPortType (i))) {
	  tmp->u.e.l = _expr_bw_adjust (H, -1, args->u.e.l, s);
	}
	else {
	  tmp->u.e.l = _expr_bw_adjust (H, TypeFactory::bitWidth (fn->getPortType (i)), args->u.e.l, s);
	}
	args = args->u.e.r;
	i++;
      }
    }
    lw = _getbw (H, e);
    if (needed_width != -1 && lw > needed_width) {
      // wrap it in an integer
      ret = _wrapint (ret, needed_width);
    }
    break;

  case E_CONCAT:
    {
      lw = _getbw (H, e);
      if (needed_width != -1 && lw > needed_width) {
	// we have extra bits! we skip as much of the concat as we can
	while (e) {
	  int x = _getbw (H, e->u.e.l);
	  if (lw - x < needed_width) {
	    break;
	  }
	  lw = lw - x;
	  e = e->u.e.r;
	}
	Assert (e, "What is going on?!");
      }
      Expr *tmp = ret;
      do {
	tmp->u.e.l = _expr_bw_adjust (H, -1, e->u.e.l, s);
	if (e->u.e.r) {
	  NEW (tmp->u.e.r, Expr);
	  tmp->u.e.r->type = E_CONCAT;
	  tmp->u.e.r->u.e.r = NULL;
	  tmp = tmp->u.e.r;
	}
	e = e->u.e.r;
      } while (e);
      if (needed_width != -1 && (lw > needed_width)) {
	ret = _wrapint (ret, needed_width);
      }
    }
    break;

  case E_VAR:
    FREE (ret);
    ret = expr_dup (e);
    if (needed_width != -1 && needed_width < _getbw (H, e)) {
      ret = _wrapint (ret, needed_width);
    }
    break;

  case E_INT:
    FREE (ret);
    ret = expr_dup (e);
    break;

  case E_REAL:
    Assert (0, "What?");
    break;

  case E_TRUE:
  case E_FALSE:
    break;

  case E_SELF:
  case E_SELF_ACK:
    lw = _getbw (H, e);
    if (needed_width != -1 && needed_width < lw) {
      ret = _wrapint (ret, needed_width);
    }
    break;

  case E_STRUCT_REF:
    Assert (0, "This should be called after unstruct()");
    break;
    
  default:
    fatal_error ("Unknown expression type (%d)!", e->type);
    break;
  }
  return ret;
}


Expr *expr_bw_adjust (int needed_width, Expr *e, Scope *s)
{
  struct pHashtable *pH;
  int tmp;
  Expr *f;

  if (!e) return NULL;
  
  pH = phash_new (4);

  tmp = _expr_bw_calc (pH, e, s);
  f = e;
  e = _expr_bw_adjust (pH, needed_width, f, s);
  expr_ex_free (f);
  if (needed_width != -1) {
    if (e->type == E_BUILTIN_INT && e->u.e.r) {
      if (e->u.e.r->u.ival.v >= needed_width) {
	f = e->u.e.l;
	e->u.e.l = NULL;
	expr_ex_free (e);
	e = f;
      }
    }
  }
  phash_free (pH);
  return e;
}

/*------------------------------------------------------------------------
 *
 * Array Expressions
 *
 *------------------------------------------------------------------------
 */
AExpr::AExpr (Expr *e)
{
  r = NULL;
  l = (AExpr *)e;
  t = AExpr::EXPR;
}

AExpr::AExpr (ActId *id)
{
  Expr *e;
  e = act_expr_var (id);

  r = NULL;
  l = (AExpr *)e;
  t = AExpr::EXPR;
}

AExpr::AExpr (AExpr::type typ, AExpr *inl, AExpr *inr)
{
  t = typ;
  l = inl;
  r = inr;
}

AExpr::~AExpr ()
{
  if (t != AExpr::EXPR) {
    if (l) {
      delete l;
    }
    if (r) {
      delete r;
    }
  }
  else {
    /* YYY: hmm... expression memory management */
    Expr *e = (Expr *)l;
    if (e->type == E_SUBRANGE || e->type == E_TYPE || e->type == E_ARRAY) {
      FREE (e);
    }
    else {
      expr_ex_free (e);
    } 
  }
}

