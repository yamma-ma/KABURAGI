#ifndef _MSC_VER
#include <stdint.h>
#endif

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define as 0 or 1 according to the floating point format suported by the
   machine */
#define HAVE_IEEEFP 1

/* Define to 1 if you have the `jbg_newlen' function. */
#define HAVE_JBG_NEWLEN 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
/* #define HAVE_IO_H 1 */

/* Define to 1 if you have the <search.h> header file. */
#define HAVE_SEARCH_H 1

/* Define to 1 if you have the `setmode' function. */
#define HAVE_SETMODE 1

/* The size of a `int', as computed by sizeof. */
#ifdef _MSC_VER
# if defined (_M_IA64) || defined(_M_X64)
#  define SIZEOF_INT 8
# else
#  define SIZEOF_INT 4
# endif
#else
# define SIZEOF_INT 8
#endif

/* The size of a `long', as computed by sizeof. */
#ifdef _MSC_VER
# define SIZEOF_LONG 4
#else
# define SIZEOF_LONG 8
#endif

/* Signed 64-bit type formatter */
#define TIFF_INT64_FORMAT "%I64d"

/* Signed 64-bit type */
#ifdef _MSC_VER
# define TIFF_INT64_T signed __int64
#else
# define TIFF_INT64_T int64_t
#endif

/* Unsigned 64-bit type formatter */
#define TIFF_UINT64_FORMAT "%I64u"

/* Unsigned 64-bit type */
#ifdef _MSC_VER
# define TIFF_UINT64_T unsigned __int64
#else
# define TIFF_UINT64_T uint64_t
#endif

/* Set the native cpu bit order */
#define HOST_FILLORDER FILLORDER_LSB2MSB

#ifdef _MSC_VER
# define snprintf _snprintf
#endif

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
# ifndef inline
#  define inline __inline
# endif
#endif

#define lfind _lfind
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
