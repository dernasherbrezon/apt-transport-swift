/* include/config.h.  Generated from config.h.in by configure.  */
/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* The following 4 are only used by inttypes.h shim if the system lacks
   inttypes.h */
/* The number of bytes in a usigned char.  */
/* #undef SIZEOF_CHAR */

/* The number of bytes in a unsigned int.  */
/* #undef SIZEOF_INT */

/* The number of bytes in a unsigned long.  */
/* #undef SIZEOF_LONG */

/* The number of bytes in a unsigned short.  */
/* #undef SIZEOF_SHORT */

/* Define if we have the timegm() function */
#define HAVE_TIMEGM 1

/* These two are used by the statvfs shim for glibc2.0 and bsd */
/* Define if we have sys/vfs.h */
/* #undef HAVE_VFS_H */

/* Define if we have sys/mount.h */
/* #undef HAVE_MOUNT_H */

/* Define if we have enabled pthread support */
/* #undef HAVE_PTHREAD */

/* If there is no socklen_t, define this for the netdb shim */
/* #undef NEED_SOCKLEN_T_DEFINE */

/* Define the arch name string */
#define COMMON_ARCH "darwin-i386"

/* The version number string */
#define VERSION "0.7.20.2"

/* The package name string */
#define PACKAGE "apt"
