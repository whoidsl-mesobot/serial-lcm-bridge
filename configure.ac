AC_INIT([serial-lcm-bridge],
	m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[https://bitbucket.org/bluesquall/serial-lcm-bridge/issues],
	[],
	[https://bitbucket.org/bluesquall/serial-lcm-bridge],
)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects dist-xz -Wall -Werror foreign])
AC_CONFIG_FILES([Makefile])

PKG_CHECK_MODULES(LCM, lcm >= 1.3.0)
AC_SUBST(LCM_LIBS)
AC_SUBST(LCM_CLFAGS)
AC_SUBST(LCM_VERSION)
AC_SEARCH_LIBS([lcm_create],[lcm])

AC_CHECK_PROG([LCMGEN], [lcm-gen], [lcm-gen], [/bin/false])
AS_IF([test "${LCMGEN}" == "x/bin/false"],
      AC_MSG_ERROR([LCM generator `lcm-gen` not found])])

AC_CHECK_PROG([RONN], [ronn], [ronn], [/bin/false])
AM_CONDITIONAL([HAVE_RONN],[test "x${RONN}" != "x/bin/false"])
AS_IF([test "x${RONN}" == "x/bin/false"],
      AC_MSG_WARN([ronn not found; will not generate manpages]) )

AC_PROG_CC
AC_OUTPUT
