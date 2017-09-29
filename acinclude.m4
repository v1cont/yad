dnl Disable deprecated GTK+ features
AC_DEFUN([GTK_DISABLE_DEPRECATED],
[
	AC_ARG_ENABLE(deprecated, [AC_HELP_STRING([--disable-deprecated],
					[Disable deprecated GTK functions])], 
			,[enable_deprecated=yes])


	if test x$enable_deprecated = xyes
	then
		DISABLE_DEPRECATED=
	else
		DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE"
	fi
	
	AC_SUBST(DISABLE_DEPRECATED)
])
