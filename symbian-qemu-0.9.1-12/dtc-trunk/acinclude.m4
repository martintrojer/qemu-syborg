dnl Useful autoconf macros

# --enable-maintainer-mode is disabled by default
AC_DEFUN([CSL_AC_MAINTAINER_MODE],[
  AC_ARG_ENABLE(maintainer-mode,
    AS_HELP_STRING([--enable-maintainer-mode],
	  	   [Enable maintainer mode]),
    MAINTAINER_MODE=$enableval,
    MAINTAINER_MODE=no)
  AC_SUBST(MAINTAINER_MODE)
])

# --with-pkgversion=VERSION
AC_DEFUN([CSL_AC_PKGVERSION],[
  AC_ARG_WITH(pkgversion,
    AS_HELP_STRING([--with-pkgversion=PKG],
                   [Add PKG to the version string]),
    [case "$withval" in
      (yes) AC_MSG_ERROR([package version not specified]) ;;
      (no)  PKGVERSION= ;;
      (*)   PKGVERSION=" ($withval)" ;;
     esac],
    PKGVERSION=
  )
  AC_SUBST(PKGVERSION)
])

# --with-bugurl=URL
AC_DEFUN([CSL_AC_BUGURL],[
  AC_ARG_WITH(bugurl,
    AS_HELP_STRING([--with-bugurl=URL],
                   [Direct users to URL to report a bug]),
    [case "$withval" in
      (yes) AC_MSG_ERROR([bug URL not specified]) ;;
      (no)  BUGURL="" ;; 
      (*)   BUGURL="<URL:$withval>" ;;
     esac],
     BUGURL=""
  )
  AC_SUBST(BUGURL)
])
