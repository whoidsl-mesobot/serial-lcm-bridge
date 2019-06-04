AC_INIT([serial-lcm-bridge], [0.2],
	[https://bitbucket.org/bluesquall/serial-lcm-bridge/issues])
AM_INIT_AUTOMAKE([subdir-objects dist-xz -Wall -Werror foreign])

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
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
