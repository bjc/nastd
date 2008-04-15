#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "../include/nastd.h"

#include "include/nastdxs.def"

static int
not_here(char *s)
{
    croak("%s not implemented on this architecture", s);
    return -1;
}

static double
constant(char *name, int arg)
{
    errno = 0;
    switch (*name) {
    case 'A':
	if (strEQ(name, "NAST_OK"))
		return NAST_OK;
	else if (strEQ(name, "NAST_SERVER_GONE"))
		return NAST_SERVER_GONE;
	else if (strEQ(name, "NAST_NOMEM"))
		return NAST_NOMEM;
	else if (strEQ(name, "NAST_UNNKNOWN_RESPONSE"))
		return NAST_UNKNOWN_RESPONSE;
	else if (strEQ(name, "NAST_TIMEDOUT"))
		return NAST_TIMEDOUT;
	else if (strEQ(name, "NAST_UNKNOWN_OPT"))
		return NAST_UNKNOWN_OPT;
	else if (strEQ(name, "NAST_SERVER_ERR"))
		return NAST_SERVER_ERR;
	break;
    case 'B':
	break;
    case 'C':
	break;
    case 'D':
	break;
    case 'E':
	break;
    case 'F':
	break;
    case 'G':
	break;
    case 'H':
	break;
    case 'I':
	break;
    case 'J':
	break;
    case 'K':
	break;
    case 'L':
	break;
    case 'M':
	break;
    case 'N':
	break;
    case 'O':
	break;
    case 'P':
	break;
    case 'Q':
	break;
    case 'R':
	break;
    case 'S':
	break;
    case 'T':
	break;
    case 'U':
	break;
    case 'V':
	break;
    case 'W':
	break;
    case 'X':
	break;
    case 'Y':
	break;
    case 'Z':
	break;
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}


MODULE = NASTD		PACKAGE = NASTD		

PROTOTYPES: ENABLE

double
constant(name,arg)
	char *		name
	int		arg

nasth *
nast_sphincter_new(...)
	PREINIT:
	STRLEN n_a;

	CODE:
	nasth *sphincter;

	if (items == 0)
		sphincter = nast_sphincter_new(SvPV(ST(0), n_a));
	else
		sphincter = nast_sphincter_new(NULL);

	if (sphincter == NULL)
		XSRETURN_UNDEF;

	RETVAL = sphincter;

	OUTPUT:
	RETVAL

void
nast_sphincter_close(sphincter)
	nasth *sphincter

int
nast_options_get(sphincter)
	nasth *sphincter

	PREINIT:
	nast_options nast_opts;

	PPCODE:
	if (nast_options_get(sphincter, &nast_opts) != -1) {
		if (nast_opts.use_qcache == NASTFALSE)
			XPUSHs(sv_2mortal(newSViv(0)));
		else
			XPUSHs(sv_2mortal(newSViv(1)));
		if (nast_opts.use_localdb == NASTFALSE)
			XPUSHs(sv_2mortal(newSViv(0)));
		else
			XPUSHs(sv_2mortal(newSViv(1)));
		if (nast_opts.fallthrough_async == NASTFALSE)
			XPUSHs(sv_2mortal(newSViv(0)));
		else
			XPUSHs(sv_2mortal(newSViv(1)));
		if (nast_opts.always_fallthrough == NASTFALSE)
			XPUSHs(sv_2mortal(newSViv(0)));
		else
			XPUSHs(sv_2mortal(newSViv(1)));
		if (nast_opts.fail_once == NASTFALSE)
			XPUSHs(sv_2mortal(newSViv(0)));
		else
			XPUSHs(sv_2mortal(newSViv(1)));
		if (nast_opts.no_fallthrough == NASTFALSE)
			XPUSHs(sv_2mortal(newSViv(0)));
		else
			XPUSHs(sv_2mortal(newSViv(1)));
	}

int
nast_options_set(sphincter, ...)
	nasth *sphincter

	PREINIT:
	nast_options nast_opts;
	int i;

	CODE:
	if (items == 8) {
		if (SvTRUE(ST(1)))
			nast_opts.use_qcache = NASTTRUE;
		else
			nast_opts.use_qcache = NASTFALSE;
		if (SvTRUE(ST(2)))
			nast_opts.use_localdb = NASTTRUE;
		else
			nast_opts.use_localdb = NASTFALSE;
		if (SvTRUE(ST(3)))
			nast_opts.fallthrough_async = NASTTRUE;
		else
			nast_opts.fallthrough_async = NASTFALSE;
		if (SvTRUE(ST(4)))
			nast_opts.always_fallthrough = NASTTRUE;
		else
			nast_opts.always_fallthrough = NASTFALSE;
		if (SvTRUE(ST(5)))
			nast_opts.fail_once = NASTTRUE;
		else
			nast_opts.fail_once = NASTFALSE;
		if (SvTRUE(ST(6)))
			nast_opts.no_fallthrough = NASTTRUE;
		else
			nast_opts.no_fallthrough = NASTFALSE;

		RETVAL = nast_options_set(sphincter, &nast_opts);
	} else
		RETVAL = -1;

	OUTPUT:
	RETVAL

SV *
nast_get_result(sphincter)
	nasth *sphincter

	PREINIT:
	nast_array *aa;
	int i;

	PPCODE:
	aa = nast_get_result(sphincter);
	if (aa != NULL) {
		EXTEND(SP, aa->nitems);

		for (i = 0; i < aa->nitems; i++) {
			SV *item;

			item = newSVpv(aa->items[i]->strdata,
				       aa->items[i]->strlen);
			PUSHs(sv_2mortal(item));
		}
		nast_free_result(aa);
	}

int
interface_nast_qcmd(sphincter, key)
	nasth *sphincter
	const char *key
INTERFACE:
	nast_add nast_del nast_get

int
nast_upd(sphincter, key, ...)
	nasth *sphincter
	const char *key

	PREINIT:
	nast_array *aa;
	int i;

	CODE:
	aa = nast_array_new();
	if (aa != NULL) {
		for (i = 2; i < items; i++) {
			char *str;
			STRLEN len;

			str = (char *)SvPV(ST(i), len);
			nast_array_add(aa, len, str);
		}
		RETVAL = nast_upd(sphincter, key, aa);
	} else {
		RETVAL = -1;
	}
	nast_array_delete(aa);

	OUTPUT:
	RETVAL

int
nast_geterr(sphincter)
	nasth *sphincter

char *
nast_errmsg(sphincter)
	nasth *sphincter

int
nast_stats(sphincter)
	nasth *sphincter
