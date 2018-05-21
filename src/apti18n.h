/* include/apti18n.h.  Generated from apti18n.h.in by configure.  */
// -*- mode: cpp; mode: fold -*-
// $Id: apti18n.h.in,v 1.6 2003/01/11 07:18:18 jgg Exp $
/* Internationalization macros for apt. This header should be included last
   in each C file. */

// Set by autoconf
/* #undef USE_NLS */

#ifdef USE_NLS
// apt will use the gettext implementation of the C library
# include <libintl.h>
# ifdef APT_DOMAIN
#   define _(x) dgettext(APT_DOMAIN,x)
# else
#   define _(x) gettext(x)
# endif
# define N_(x) x
#else
// apt will not use any gettext
# define setlocale(a, b)
# define textdomain(a)
# define bindtextdomain(a, b)
# define _(x) x
# define N_(x) x
#endif
