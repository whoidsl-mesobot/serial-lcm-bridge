AC_INIT([serial-lcm-bridge], [0.2], [m.j.stanway@alum.mit.edu])
AM_INIT_AUTOMAKE([subdir-objects dist-xz -Wall -Werror foreign])

PKG_CHECK_MODULES(LCM, lcm >= 1.3.0)
AC_SUBST(LCM_LIBS)
AC_SUBST(LCM_CLFAGS)
AC_SUBST(LCM_VERSION)

#AC_CHECK_PROG([LCMGEN],[lcm-gen],[lcm-gen])
AC_ARG_WITH([lcm-gen],
  [AS_HELP_STRING([--with-lcm-gen=/path/to/lcm-gen],
    [Location of the LCM code generator tool lcm-gen.])],
  [LCMGEN="$withval"],
  [AS_IF([test "x${LCMGEN}" == "x"], [AC_PATH_PROG([LCMGEN], [lcm-gen], [no])])]
)
AS_IF([test "${LCMGEN}" == "no"], AC_MSG_ERROR([LCM generator "lcm-gen" not found])])

AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
