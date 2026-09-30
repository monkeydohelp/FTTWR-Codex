#pragma once

/* Compatibility shim for the Civ4SDK Windows headers and VC++ 2003.
 * SAL is static-analysis metadata only; these no-op definitions have no
 * runtime effect. */
#ifndef CIV4SDK_SAL_COMPAT_H
#define CIV4SDK_SAL_COMPAT_H

#define __SAL_H_FULL_VER 140050727

#define __null
#define __notnull
#define __maybenull
#define __readonly
#define __notreadonly
#define __valid
#define __notvalid
#define __readableTo(size)
#define __writableTo(size)
#define __pre
#define __post
#define __deref
#define __exceptthat
#define __inner
#define __outer
#define __in
#define __out
#define __inout
#define __ecount(size)
#define __bcount(size)
#define __count(size)
#define __success(expr)
#define __checkReturn
#define __typefix(ctype)
#define __analysis_assume(expr)
#define __nullterminated
#define __callback
#define __inner_checkReturn

#define __in_opt
#define __out_opt
#define __inout_opt
#define __deref_out
#define __deref_out_opt
#define __deref_inout
#define __deref_inout_opt
#define __deref_opt_out_opt
#define __deref_opt_inout_opt

#define __in_ecount(size)
#define __in_bcount(size)
#define __in_ecount_opt(size)
#define __in_bcount_opt(size)
#define __out_ecount(size)
#define __out_bcount(size)
#define __out_ecount_opt(size)
#define __out_bcount_opt(size)
#define __inout_ecount(size)
#define __inout_bcount(size)
#define __inout_ecount_opt(size)
#define __inout_bcount_opt(size)

#define __elem_readableTo(size)
#define __elem_writableTo(size)
#define __bcount_opt(size)
#define __reserved
#define __out_ecount_part(size,count)
#define __out_bcount_part(size,count)
#define __out_ecount_part_opt(size,count)
#define __out_bcount_part_opt(size,count)
#define __out_ecount_full(size)
#define __out_bcount_full(size)
#define __in_ecount_part(size,count)
#define __in_bcount_part(size,count)
#define __inout_ecount_part(size,count)
#define __inout_bcount_part(size,count)
#define __refparam
#define __deref_opt_out
#define __inner_control_entrypoint(category)
#define __deref_opt_out_bcount_full(size)
#define __format_string
#define __byte_writableTo(size)
#define __inout_bcount_part_opt(size,count)

#endif
