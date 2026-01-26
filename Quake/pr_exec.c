/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

static const char *const pr_opnames[] =
{
	"DONE",

	"MUL_F",
	"MUL_V",
	"MUL_FV",
	"MUL_VF",

	"DIV",

	"ADD_F",
	"ADD_V",

	"SUB_F",
	"SUB_V",

	"EQ_F",
	"EQ_V",
	"EQ_S",
	"EQ_E",
	"EQ_FNC",

	"NE_F",
	"NE_V",
	"NE_S",
	"NE_E",
	"NE_FNC",

	"LE",
	"GE",
	"LT",
	"GT",

	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",

	"ADDRESS",

	"STORE_F",
	"STORE_V",
	"STORE_S",
	"STORE_ENT",
	"STORE_FLD",
	"STORE_FNC",

	"STOREP_F",
	"STOREP_V",
	"STOREP_S",
	"STOREP_ENT",
	"STOREP_FLD",
	"STOREP_FNC",

	"RETURN",

	"NOT_F",
	"NOT_V",
	"NOT_S",
	"NOT_ENT",
	"NOT_FNC",

	"IF",
	"IFNOT",

	"CALL0",
	"CALL1",
	"CALL2",
	"CALL3",
	"CALL4",
	"CALL5",
	"CALL6",
	"CALL7",
	"CALL8",

	"STATE",

	"GOTO",

	"AND",
	"OR",

	"BITAND",
	"BITOR",

	/* Hexen II opcodes */
	"MULSTORE_F",
	"MULSTORE_V",
	"MULSTOREP_F",
	"MULSTOREP_V",

	"DIVSTORE_F",
	"DIVSTOREP_F",

	"ADDSTORE_F",
	"ADDSTORE_V",
	"ADDSTOREP_F",
	"ADDSTOREP_V",

	"SUBSTORE_F",
	"SUBSTORE_V",
	"SUBSTOREP_F",
	"SUBSTOREP_V",

	"FETCH_GBL_F",
	"FETCH_GBL_V",
	"FETCH_GBL_S",
	"FETCH_GBL_E",
	"FETCH_GBL_FNC",

	"CSTATE",
	"CWSTATE",

	"THINKTIME",

	"BITSET",
	"BITSETP",
	"BITCLR",
	"BITCLRP",

	"RAND0",
	"RAND1",
	"RAND2",
	"RANDV0",
	"RANDV1",
	"RANDV2",

	"SWITCH_F",
	"SWITCH_V",
	"SWITCH_S",
	"SWITCH_E",
	"SWITCH_FNC",

	"CASE",
	"CASERANGE"
};

static const char *const pr_extnames[QCEXT_COUNT] =
{
	"STD_QC",

	#define QCEXTENSION(name) #name,
	QCEXTENSIONS_ALL
	#undef QCEXTENSION
};

/*
=================
PR_FindExtensionByName
=================
*/
int PR_FindExtensionByName (const char *name)
{
	int i;
	for (i = 1; i < QCEXT_COUNT; i++)
		if (!strcmp (name, pr_extnames[i]))
			return i;
	return 0;
}

const char *PR_GlobalString (int ofs);
const char *PR_GlobalStringNoContents (int ofs);


//=============================================================================

/*
=================
PR_PrintStatement
=================
*/
static void PR_PrintStatement (dstatement_t *s)
{
	int	i;

	if ((unsigned int)s->op < Q_COUNTOF(pr_opnames))
	{
		Con_Printf("%s ", pr_opnames[s->op]);
		i = strlen(pr_opnames[s->op]);
		for ( ; i < 10; i++)
			Con_Printf(" ");
	}

	if (s->op == OP_IF || s->op == OP_IFNOT)
		Con_Printf("%sbranch %i", PR_GlobalString(s->a), s->b);
	else if (s->op == OP_GOTO)
	{
		Con_Printf("branch %i", s->a);
	}
	else if ((unsigned int)(s->op-OP_STORE_F) < 6)
	{
		Con_Printf("%s", PR_GlobalString(s->a));
		Con_Printf("%s", PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			Con_Printf("%s", PR_GlobalString(s->a));
		if (s->b)
			Con_Printf("%s", PR_GlobalString(s->b));
		if (s->c)
			Con_Printf("%s", PR_GlobalStringNoContents(s->c));
	}
	Con_Printf("\n");
}

/*
============
PR_StackTrace
============
*/
static void PR_StackTrace (void)
{
	int		i;
	dfunction_t	*f;

	if (qcvm->depth == 0)
	{
		Con_Printf("<NO STACK>\n");
		return;
	}

	qcvm->stack[qcvm->depth].f = qcvm->xfunction;
	for (i = qcvm->depth; i >= 0; i--)
	{
		f = qcvm->stack[i].f;
		if (!f)
		{
			Con_Printf("<NO FUNCTION>\n");
		}
		else
		{
			Con_Printf("%12s : %s\n", PR_GetString(f->s_file), PR_GetString(f->s_name));
		}
	}
}


/*
============
PR_Profile_f

============
*/
void PR_Profile_f (void)
{
	int		i, num;
	int		pmax;
	dfunction_t	*f, *best;

	if (!sv.active)
		return;

	PR_SwitchQCVM(&sv.qcvm);

	num = 0;
	do
	{
		pmax = 0;
		best = NULL;
		for (i = 0; i < qcvm->progs->numfunctions; i++)
		{
			f = &qcvm->functions[i];
			if (f->profile > pmax)
			{
				pmax = f->profile;
				best = f;
			}
		}
		if (best)
		{
			if (num < 10)
				Con_Printf("%7i %s\n", best->profile, PR_GetString(best->s_name));
			num++;
			best->profile = 0;
		}
	} while (best);

	PR_SwitchQCVM(NULL);
}


/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (const char *error, ...)
{
	va_list	argptr;
	char	string[1024];

	va_start (argptr, error);
	q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);

	PR_PrintStatement(qcvm->statements + qcvm->xstatement);
	PR_StackTrace();

	Con_Printf("%s\n", string);

	qcvm->depth = 0;	// dump the stack so host_error can shutdown functions

	Host_Error("Program error");
}

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
static int PR_EnterFunction (dfunction_t *f)
{
	int	i, j, c, o;

	qcvm->stack[qcvm->depth].s = qcvm->xstatement;
	qcvm->stack[qcvm->depth].f = qcvm->xfunction;
	qcvm->depth++;
	if (qcvm->depth >= MAX_STACK_DEPTH)
		PR_RunError("stack overflow");

	// save off any locals that the new function steps on
	c = f->locals;
	if (qcvm->localstack_used + c > LOCALSTACK_SIZE)
		PR_RunError("PR_ExecuteProgram: locals stack overflow");

	for (i = 0; i < c ; i++)
		qcvm->localstack[qcvm->localstack_used + i] = ((int *)qcvm->globals)[f->parm_start + i];
	qcvm->localstack_used += c;

	// copy parameters
	o = f->parm_start;
	for (i = 0; i < f->numparms; i++)
	{
		for (j = 0; j < f->parm_size[i]; j++)
		{
			((int *)qcvm->globals)[o] = ((int *)qcvm->globals)[OFS_PARM0 + i*3 + j];
			o++;
		}
	}

	qcvm->xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
static int PR_LeaveFunction (void)
{
	int	i, c;

	if (qcvm->depth <= 0)
		Host_Error("prog stack underflow");

	// Restore locals from the stack
	c = qcvm->xfunction->locals;
	qcvm->localstack_used -= c;
	if (qcvm->localstack_used < 0)
		PR_RunError("PR_ExecuteProgram: locals stack underflow");

	for (i = 0; i < c; i++)
		((int *)qcvm->globals)[qcvm->xfunction->parm_start + i] = qcvm->localstack[qcvm->localstack_used + i];

	// up stack
	qcvm->depth--;
	qcvm->xfunction = qcvm->stack[qcvm->depth].f;
	return qcvm->stack[qcvm->depth].s;
}

/*
====================
PR_CheckBuiltinExtension
====================
*/
static void PR_CheckBuiltinExtension (dfunction_t *func)
{
	uint32_t builtin = -func->first_statement;
	uint32_t extnum = qcvm->builtin_ext[builtin];
	uint32_t checked, advertised;

	if (!extnum)
		return;

	checked = GetBit (qcvm->checked_ext, extnum);
	advertised = GetBit (qcvm->advertised_ext, extnum);
	if (checked && advertised)
		return;

	if (GetBit (qcvm->warned_builtin[checked], builtin))
		return;
	SetBit (qcvm->warned_builtin[checked], builtin);

	Con_DWarning (checked ?
		"[%s] \"%s\" ignored when calling %s (%s: %s)\n" :
		"[%s] check \"%s\" before calling %s (%s: %s)\n",
		(qcvm == &cl.qcvm) ? "CL" : "SV",
		pr_extnames[extnum], PR_GetString (func->s_name),
		PR_GetString (qcvm->xfunction->s_file), PR_GetString (qcvm->xfunction->s_name)
	);
}

/*
====================
PR_ExecuteProgram

The interpretation main loop
====================
*/
#define OPA ((eval_t *)&qcvm->globals[(unsigned short)st->a])
#define OPB ((eval_t *)&qcvm->globals[(unsigned short)st->b])
#define OPC ((eval_t *)&qcvm->globals[(unsigned short)st->c])

/* Hexen II frame time constant (20 fps) */
#define HX_FRAME_TIME	0.05f

/* H2 entity field access macros - use dynamic offsets for H2 mode */
#define H2_ED_FLOAT(ed, ofs) (((float *)&(ed)->v)[ofs])
#define H2_ED_INT(ed, ofs) (((int *)&(ed)->v)[ofs])
#define H2_ED_FUNC(ed, ofs) (((func_t *)&(ed)->v)[ofs])

/* Hexen II switch statement types */
enum {
	H2_SWITCH_F,
	H2_SWITCH_V,
	H2_SWITCH_S,
	H2_SWITCH_E,
	H2_SWITCH_FNC
};

void PR_ExecuteProgram (func_t fnum)
{
	eval_t		*ptr;
	dstatement_t	*st;
	dfunction_t	*f, *newf;
	int profile, startprofile;
	edict_t		*ed;
	int		exitdepth;
	/* Hexen II switch statement state */
	int		h2_case_type = 0;
	float		h2_switch_float = 0;

	if (!fnum || fnum >= qcvm->progs->numfunctions)
	{
		if (pr_global_struct->self)
			ED_Print (PROG_TO_EDICT(pr_global_struct->self));
		Host_Error ("PR_ExecuteProgram: NULL function");
	}

	f = &qcvm->functions[fnum];

	qcvm->trace = false;

// make a stack frame
	exitdepth = qcvm->depth;

	st = &qcvm->statements[PR_EnterFunction(f)];
	startprofile = profile = 0;

    while (1)
    {
	st++;	/* next statement */

	if (++profile > 0x1000000) /* was 100000 */
	{
		qcvm->xstatement = st - qcvm->statements;
		PR_RunError("runaway loop error");
	}

	if (qcvm->trace)
		PR_PrintStatement(st);

	switch (st->op)
	{
	case OP_ADD_F:
		OPC->_float = OPA->_float + OPB->_float;
		break;
	case OP_ADD_V:
		OPC->vector[0] = OPA->vector[0] + OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] + OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] + OPB->vector[2];
		break;

	case OP_SUB_F:
		OPC->_float = OPA->_float - OPB->_float;
		break;
	case OP_SUB_V:
		OPC->vector[0] = OPA->vector[0] - OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] - OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] - OPB->vector[2];
		break;

	case OP_MUL_F:
		OPC->_float = OPA->_float * OPB->_float;
		break;
	case OP_MUL_V:
		OPC->_float = OPA->vector[0] * OPB->vector[0] +
			      OPA->vector[1] * OPB->vector[1] +
			      OPA->vector[2] * OPB->vector[2];
		break;
	case OP_MUL_FV:
		OPC->vector[0] = OPA->_float * OPB->vector[0];
		OPC->vector[1] = OPA->_float * OPB->vector[1];
		OPC->vector[2] = OPA->_float * OPB->vector[2];
		break;
	case OP_MUL_VF:
		OPC->vector[0] = OPB->_float * OPA->vector[0];
		OPC->vector[1] = OPB->_float * OPA->vector[1];
		OPC->vector[2] = OPB->_float * OPA->vector[2];
		break;

	case OP_DIV_F:
		OPC->_float = OPA->_float / OPB->_float;
		break;

	case OP_BITAND:
		OPC->_float = (int)OPA->_float & (int)OPB->_float;
		break;

	case OP_BITOR:
		OPC->_float = (int)OPA->_float | (int)OPB->_float;
		break;

	case OP_GE:
		OPC->_float = OPA->_float >= OPB->_float;
		break;
	case OP_LE:
		OPC->_float = OPA->_float <= OPB->_float;
		break;
	case OP_GT:
		OPC->_float = OPA->_float > OPB->_float;
		break;
	case OP_LT:
		OPC->_float = OPA->_float < OPB->_float;
		break;
	case OP_AND:
		OPC->_float = OPA->_float && OPB->_float;
		break;
	case OP_OR:
		OPC->_float = OPA->_float || OPB->_float;
		break;

	case OP_NOT_F:
		OPC->_float = !OPA->_float;
		break;
	case OP_NOT_V:
		OPC->_float = !OPA->vector[0] && !OPA->vector[1] && !OPA->vector[2];
		break;
	case OP_NOT_S:
		OPC->_float = !OPA->string || !*PR_GetString(OPA->string);
		break;
	case OP_NOT_FNC:
		OPC->_float = !OPA->function;
		break;
	case OP_NOT_ENT:
		OPC->_float = (PROG_TO_EDICT(OPA->edict) == qcvm->edicts);
		break;

	case OP_EQ_F:
		OPC->_float = OPA->_float == OPB->_float;
		break;
	case OP_EQ_V:
		OPC->_float = (OPA->vector[0] == OPB->vector[0]) &&
			      (OPA->vector[1] == OPB->vector[1]) &&
			      (OPA->vector[2] == OPB->vector[2]);
		break;
	case OP_EQ_S:
		OPC->_float = !strcmp(PR_GetString(OPA->string), PR_GetString(OPB->string));
		break;
	case OP_EQ_E:
		OPC->_float = OPA->_int == OPB->_int;
		break;
	case OP_EQ_FNC:
		OPC->_float = OPA->function == OPB->function;
		break;

	case OP_NE_F:
		OPC->_float = OPA->_float != OPB->_float;
		break;
	case OP_NE_V:
		OPC->_float = (OPA->vector[0] != OPB->vector[0]) ||
			      (OPA->vector[1] != OPB->vector[1]) ||
			      (OPA->vector[2] != OPB->vector[2]);
		break;
	case OP_NE_S:
		OPC->_float = strcmp(PR_GetString(OPA->string), PR_GetString(OPB->string));
		break;
	case OP_NE_E:
		OPC->_float = OPA->_int != OPB->_int;
		break;
	case OP_NE_FNC:
		OPC->_float = OPA->function != OPB->function;
		break;

	case OP_STORE_F:
	case OP_STORE_ENT:
	case OP_STORE_FLD:	// integers
	case OP_STORE_S:
	case OP_STORE_FNC:	// pointers
		OPB->_int = OPA->_int;
		break;
	case OP_STORE_V:
		OPB->vector[0] = OPA->vector[0];
		OPB->vector[1] = OPA->vector[1];
		OPB->vector[2] = OPA->vector[2];
		break;

	case OP_STOREP_F:
	case OP_STOREP_ENT:
	case OP_STOREP_FLD:	// integers
	case OP_STOREP_S:
	case OP_STOREP_FNC:	// pointers
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		ptr->_int = OPA->_int;
		break;
	case OP_STOREP_V:
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		ptr->vector[0] = OPA->vector[0];
		ptr->vector[1] = OPA->vector[1];
		ptr->vector[2] = OPA->vector[2];
		break;

	case OP_ADDRESS:
		{
			ed = PROG_TO_EDICT(OPA->edict);
#ifdef PARANOID
			NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
			if (ed == (edict_t *)qcvm->edicts && sv.state == ss_active)
			{
				qcvm->xstatement = st - qcvm->statements;
				PR_RunError("assignment to world entity");
			}
			// Debug: check if we're writing to OFS_PARM0
			if ((unsigned short)st->c == OFS_PARM0 / 4 || (unsigned short)st->c == OFS_PARM1 / 4)
			{
				Con_Printf("OP_ADDRESS: writing to globals[%d] (c=%d), result=%d (0x%x)\n",
					(unsigned short)st->c, (unsigned short)st->c,
					(byte *)((int *)&ed->v + OPB->_int) - (byte *)qcvm->edicts,
					(byte *)((int *)&ed->v + OPB->_int) - (byte *)qcvm->edicts);
				fflush(stdout);
			}
			OPC->_int = (byte *)((int *)&ed->v + OPB->_int) - (byte *)qcvm->edicts;
		}
		break;

	case OP_LOAD_F:
	case OP_LOAD_FLD:
	case OP_LOAD_ENT:
	case OP_LOAD_S:
	case OP_LOAD_FNC:
		ed = PROG_TO_EDICT(OPA->edict);
		NUM_FOR_EDICT(ed);	// Make sure it's in range
		OPC->_int = ((eval_t *)((int *)&ed->v + OPB->_int))->_int;
		break;

	case OP_LOAD_V:
		{
			ed = PROG_TO_EDICT(OPA->edict);
		}
		ptr = (eval_t *)((int *)&ed->v + OPB->_int);
		OPC->vector[0] = ptr->vector[0];
		OPC->vector[1] = ptr->vector[1];
		OPC->vector[2] = ptr->vector[2];
		break;

	case OP_IFNOT:
		if (!OPA->_int)
			st += st->b - 1;	/* -1 to offset the st++ */
		break;

	case OP_IF:
		if (OPA->_int)
			st += st->b - 1;	/* -1 to offset the st++ */
		break;

	case OP_GOTO:
		st += st->a - 1;		/* -1 to offset the st++ */
		break;

	case OP_CALL8:
	case OP_CALL7:
	case OP_CALL6:
	case OP_CALL5:
	case OP_CALL4:
	case OP_CALL3:
	case OP_CALL2:
	case OP_CALL1:
	case OP_CALL0:
		qcvm->xfunction->profile += profile - startprofile;
		startprofile = profile;
		qcvm->xstatement = st - qcvm->statements;
		qcvm->argc = st->op - OP_CALL0;
		if (!OPA->function)
			PR_RunError("NULL function");
		newf = &qcvm->functions[OPA->function];
		/* H2 calling convention for builtins:
		 * v1.11 (CRC 38488): ALL calls use HexenC convention (params at st->b/c)
		 * v1.12 (CRC 26905): Only BUILTIN calls use HexenC convention
		 *                    QC function calls use standard Quake convention (params in OFS_PARM0)
		 */
		/* H2 calling convention:
		 * v1.11 (CRC 38488): Uses HexenC calling convention - copy params from st->b/c
		 * v1.12 (CRC 26905): Uses standard Quake calling convention - params pre-set
		 *                    (The v1.12 progs was likely compiled with a Quake-compatible compiler)
		 */
		if (hexen2_mode && qcvm->progs && qcvm->progs->crc == 38488)
		{
			/* v1.11 only: Copy params from st->b/c to OFS_PARM0/1 */
			if (st->op >= OP_CALL1)
			{
				qcvm->globals[OFS_PARM0] = OPB->vector[0];
				qcvm->globals[OFS_PARM0 + 1] = OPB->vector[1];
				qcvm->globals[OFS_PARM0 + 2] = OPB->vector[2];
			}
			if (st->op >= OP_CALL2)
			{
				qcvm->globals[OFS_PARM1] = OPC->vector[0];
				qcvm->globals[OFS_PARM1 + 1] = OPC->vector[1];
				qcvm->globals[OFS_PARM1 + 2] = OPC->vector[2];
			}
		}
		if (newf->first_statement < 0)
		{ // Built-in function
			int i = -newf->first_statement;
			if (i >= qcvm->numbuiltins)
				PR_RunError("Bad builtin call number %d", i);
			PR_CheckBuiltinExtension (newf);
			qcvm->builtins[i]();
			break;
		}
		// Normal function
		st = &qcvm->statements[PR_EnterFunction(newf)];
		break;

	case OP_DONE:
	case OP_RETURN:
		qcvm->xfunction->profile += profile - startprofile;
		startprofile = profile;
		qcvm->xstatement = st - qcvm->statements;
		qcvm->globals[OFS_RETURN] = qcvm->globals[(unsigned short)st->a];
		qcvm->globals[OFS_RETURN + 1] = qcvm->globals[(unsigned short)st->a + 1];
		qcvm->globals[OFS_RETURN + 2] = qcvm->globals[(unsigned short)st->a + 2];
		st = &qcvm->statements[PR_LeaveFunction()];
		if (qcvm->depth == exitdepth)
		{ // Done
			return;
		}
		break;

	case OP_STATE:
		ed = PROG_TO_EDICT(pr_global_struct->self);
		ed->v.nextthink = pr_global_struct->time + 0.1;
		ed->v.frame = OPA->_float;
		ed->v.think = OPB->function;
		break;

	/* ==================== */
	/* Hexen II Opcodes     */
	/* ==================== */

	case OP_MULSTORE_F:	/* f *= f */
		OPB->_float *= OPA->_float;
		break;
	case OP_MULSTORE_V:	/* v *= f */
		OPB->vector[0] *= OPA->_float;
		OPB->vector[1] *= OPA->_float;
		OPB->vector[2] *= OPA->_float;
		break;
	case OP_MULSTOREP_F:	/* e.f *= f */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		OPC->_float = (ptr->_float *= OPA->_float);
		break;
	case OP_MULSTOREP_V:	/* e.v *= f */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		OPC->vector[0] = (ptr->vector[0] *= OPA->_float);
		OPC->vector[1] = (ptr->vector[1] *= OPA->_float);
		OPC->vector[2] = (ptr->vector[2] *= OPA->_float);
		break;

	case OP_DIVSTORE_F:	/* f /= f */
		OPB->_float /= OPA->_float;
		break;
	case OP_DIVSTOREP_F:	/* e.f /= f */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		OPC->_float = (ptr->_float /= OPA->_float);
		break;

	case OP_ADDSTORE_F:	/* f += f */
		OPB->_float += OPA->_float;
		break;
	case OP_ADDSTORE_V:	/* v += v */
		OPB->vector[0] += OPA->vector[0];
		OPB->vector[1] += OPA->vector[1];
		OPB->vector[2] += OPA->vector[2];
		break;
	case OP_ADDSTOREP_F:	/* e.f += f */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		OPC->_float = (ptr->_float += OPA->_float);
		break;
	case OP_ADDSTOREP_V:	/* e.v += v */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		OPC->vector[0] = (ptr->vector[0] += OPA->vector[0]);
		OPC->vector[1] = (ptr->vector[1] += OPA->vector[1]);
		OPC->vector[2] = (ptr->vector[2] += OPA->vector[2]);
		break;

	case OP_SUBSTORE_F:	/* f -= f */
		OPB->_float -= OPA->_float;
		break;
	case OP_SUBSTORE_V:	/* v -= v */
		OPB->vector[0] -= OPA->vector[0];
		OPB->vector[1] -= OPA->vector[1];
		OPB->vector[2] -= OPA->vector[2];
		break;
	case OP_SUBSTOREP_F:	/* e.f -= f */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		OPC->_float = (ptr->_float -= OPA->_float);
		break;
	case OP_SUBSTOREP_V:	/* e.v -= v */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		OPC->vector[0] = (ptr->vector[0] -= OPA->vector[0]);
		OPC->vector[1] = (ptr->vector[1] -= OPA->vector[1]);
		OPC->vector[2] = (ptr->vector[2] -= OPA->vector[2]);
		break;

	case OP_FETCH_GBL_F:
	case OP_FETCH_GBL_S:
	case OP_FETCH_GBL_E:
	case OP_FETCH_GBL_FNC:
	  {
		int i = (int)OPB->_float;
		if (i < 0 || i > G_INT((unsigned short)st->a - 1))
		{
			qcvm->xstatement = st - qcvm->statements;
			PR_RunError("array index out of bounds: %d", i);
		}
		ptr = (eval_t *)&qcvm->globals[(unsigned short)st->a + i];
		OPC->_int = ptr->_int;
	  }
		break;
	case OP_FETCH_GBL_V:
	  {
		int i = (int)OPB->_float;
		if (i < 0 || i > G_INT((unsigned short)st->a - 1))
		{
			qcvm->xstatement = st - qcvm->statements;
			PR_RunError("array index out of bounds: %d", i);
		}
		ptr = (eval_t *)&qcvm->globals[(unsigned short)st->a + (i * 3)];
		OPC->vector[0] = ptr->vector[0];
		OPC->vector[1] = ptr->vector[1];
		OPC->vector[2] = ptr->vector[2];
	  }
		break;

	case OP_CSTATE:	/* Cycle state - frame animation */
	  {
		int startFrame, endFrame;
		float curFrame;
		ed = PROG_TO_EDICT(pr_global_struct->self);
		H2_ED_FLOAT(ed, h2_globals.fields.nextthink) = pr_global_struct->time + HX_FRAME_TIME;
		H2_ED_FUNC(ed, h2_globals.fields.think) = qcvm->xfunction - qcvm->functions;
		if (h2_globals.cycle_wrapped) *h2_globals.cycle_wrapped = false;
		startFrame = (int)OPA->_float;
		endFrame = (int)OPB->_float;
		curFrame = H2_ED_FLOAT(ed, h2_globals.fields.frame);
		if (startFrame <= endFrame)
		{
			if (curFrame < startFrame || curFrame > endFrame)
				H2_ED_FLOAT(ed, h2_globals.fields.frame) = startFrame;
			else
			{
				curFrame++;
				H2_ED_FLOAT(ed, h2_globals.fields.frame) = curFrame;
				if (curFrame > endFrame)
				{
					if (h2_globals.cycle_wrapped) *h2_globals.cycle_wrapped = true;
					H2_ED_FLOAT(ed, h2_globals.fields.frame) = startFrame;
				}
			}
		}
		else
		{
			if (curFrame > startFrame || curFrame < endFrame)
				H2_ED_FLOAT(ed, h2_globals.fields.frame) = startFrame;
			else
			{
				curFrame--;
				H2_ED_FLOAT(ed, h2_globals.fields.frame) = curFrame;
				if (curFrame < endFrame)
				{
					if (h2_globals.cycle_wrapped) *h2_globals.cycle_wrapped = true;
					H2_ED_FLOAT(ed, h2_globals.fields.frame) = startFrame;
				}
			}
		}
	  }
		break;

	case OP_CWSTATE:	/* Cycle weapon state - weapon frame animation */
	  {
		int startFrame, endFrame;
		float curFrame;
		ed = PROG_TO_EDICT(pr_global_struct->self);
		H2_ED_FLOAT(ed, h2_globals.fields.nextthink) = pr_global_struct->time + HX_FRAME_TIME;
		H2_ED_FUNC(ed, h2_globals.fields.think) = qcvm->xfunction - qcvm->functions;
		if (h2_globals.cycle_wrapped) *h2_globals.cycle_wrapped = false;
		startFrame = (int)OPA->_float;
		endFrame = (int)OPB->_float;
		curFrame = H2_ED_FLOAT(ed, h2_globals.fields.weaponframe);
		if (startFrame <= endFrame)
		{
			if (curFrame < startFrame || curFrame > endFrame)
				H2_ED_FLOAT(ed, h2_globals.fields.weaponframe) = startFrame;
			else
			{
				curFrame++;
				H2_ED_FLOAT(ed, h2_globals.fields.weaponframe) = curFrame;
				if (curFrame > endFrame)
				{
					if (h2_globals.cycle_wrapped) *h2_globals.cycle_wrapped = true;
					H2_ED_FLOAT(ed, h2_globals.fields.weaponframe) = startFrame;
				}
			}
		}
		else
		{
			if (curFrame > startFrame || curFrame < endFrame)
				H2_ED_FLOAT(ed, h2_globals.fields.weaponframe) = startFrame;
			else
			{
				curFrame--;
				H2_ED_FLOAT(ed, h2_globals.fields.weaponframe) = curFrame;
				if (curFrame < endFrame)
				{
					if (h2_globals.cycle_wrapped) *h2_globals.cycle_wrapped = true;
					H2_ED_FLOAT(ed, h2_globals.fields.weaponframe) = startFrame;
				}
			}
		}
	  }
		break;

	case OP_THINKTIME:
		ed = PROG_TO_EDICT(OPA->edict);
		if (ed == (edict_t *)qcvm->edicts && sv.state == ss_active)
		{
			qcvm->xstatement = st - qcvm->statements;
			PR_RunError("assignment to world entity");
		}
		H2_ED_FLOAT(ed, h2_globals.fields.nextthink) = pr_global_struct->time + OPB->_float;
		break;

	case OP_BITSET:		/* f (+) f  - set bits */
		OPB->_float = (int)OPB->_float | (int)OPA->_float;
		break;
	case OP_BITSETP:	/* e.f (+) f */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		ptr->_float = (int)ptr->_float | (int)OPA->_float;
		break;
	case OP_BITCLR:		/* f (-) f  - clear bits */
		OPB->_float = (int)OPB->_float & ~((int)OPA->_float);
		break;
	case OP_BITCLRP:	/* e.f (-) f */
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		ptr->_float = (int)ptr->_float & ~((int)OPA->_float);
		break;

	case OP_RAND0:	/* random() - 0 to 1 */
	  {
		float val = rand() * (1.0f / RAND_MAX);
		G_FLOAT(OFS_RETURN) = val;
	  }
		break;
	case OP_RAND1:	/* random(f) - 0 to f */
	  {
		float val = rand() * (1.0f / RAND_MAX) * OPA->_float;
		G_FLOAT(OFS_RETURN) = val;
	  }
		break;
	case OP_RAND2:	/* random(f, f) - min to max */
	  {
		float val;
		if (OPA->_float < OPB->_float)
			val = OPA->_float + (rand() * (1.0f / RAND_MAX) * (OPB->_float - OPA->_float));
		else
			val = OPB->_float + (rand() * (1.0f / RAND_MAX) * (OPA->_float - OPB->_float));
		G_FLOAT(OFS_RETURN) = val;
	  }
		break;
	case OP_RANDV0:	/* randomv() - vector 0 to 1 */
	  {
		float *retptr = &G_FLOAT(OFS_RETURN);
		*retptr++ = rand() * (1.0f / RAND_MAX);
		*retptr++ = rand() * (1.0f / RAND_MAX);
		*retptr   = rand() * (1.0f / RAND_MAX);
	  }
		break;
	case OP_RANDV1:	/* randomv(v) - vector 0 to v */
	  {
		float *retptr = &G_FLOAT(OFS_RETURN);
		*retptr++ = rand() * (1.0f / RAND_MAX) * OPA->vector[0];
		*retptr++ = rand() * (1.0f / RAND_MAX) * OPA->vector[1];
		*retptr   = rand() * (1.0f / RAND_MAX) * OPA->vector[2];
	  }
		break;
	case OP_RANDV2:	/* randomv(v, v) - vector min to max */
	  {
		float val;
		int i;
		float *retptr = &G_FLOAT(OFS_RETURN);
		for (i = 0; i < 3; i++)
		{
			if (OPA->vector[i] < OPB->vector[i])
				val = OPA->vector[i] + (rand() * (1.0f / RAND_MAX) * (OPB->vector[i] - OPA->vector[i]));
			else
				val = OPB->vector[i] + (rand() * (1.0f / RAND_MAX) * (OPA->vector[i] - OPB->vector[i]));
			*retptr++ = val;
		}
	  }
		break;

	case OP_SWITCH_F:
		h2_case_type = H2_SWITCH_F;
		h2_switch_float = OPA->_float;
		st += (short)st->b - 1;	/* -1 to offset the st++ */
		break;
	case OP_SWITCH_V:
	case OP_SWITCH_S:
	case OP_SWITCH_E:
	case OP_SWITCH_FNC:
		qcvm->xstatement = st - qcvm->statements;
		PR_RunError("switch type %d not implemented", st->op - OP_SWITCH_F);
		break;

	case OP_CASE:
		if (h2_case_type == H2_SWITCH_F)
		{
			if (h2_switch_float == OPA->_float)
				st += (short)st->b - 1;	/* -1 to offset the st++ */
		}
		else
		{
			qcvm->xstatement = st - qcvm->statements;
			PR_RunError("case for switch type %d not implemented", h2_case_type);
		}
		break;

	case OP_CASERANGE:
		if (h2_case_type != H2_SWITCH_F)
		{
			qcvm->xstatement = st - qcvm->statements;
			PR_RunError("caserange requires float switch");
		}
		if ((h2_switch_float >= OPA->_float) && (h2_switch_float <= OPB->_float))
			st += (short)st->c - 1;	/* -1 to offset the st++ */
		break;

	default:
		qcvm->xstatement = st - qcvm->statements;
		PR_RunError("Bad opcode %i", st->op);
	}
    }	/* end of while(1) loop */
}

#undef OPA
#undef OPB
#undef OPC
