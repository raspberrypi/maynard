AC_PREREQ([2.64])
AC_INIT([weston-gtk-shell],
        [0.1.0])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG()

PKG_CHECK_MODULES([GTK], [wayland-client >= 1.0.2 weston >= 1.3.0 gtk+-3.0 >= 3.7 libgnome-menu-3.0])

WAYLAND_SCANNER_RULES(['$(top_srcdir)/protocol'])

AC_CONFIG_FILES([Makefile
		 shell/Makefile
		 protocol/Makefile
		 ])
AC_OUTPUT
