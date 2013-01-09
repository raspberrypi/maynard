libexec_PROGRAMS = weston-desktop-shell

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = $(CLIENT_CFLAGS) $(GTK_CFLAGS)

weston_desktop_shell_SOURCES =			\
	gtk-shell.c				\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c
weston_desktop_shell_LDADD = $(GTK_LIBS)

BUILT_SOURCES =					\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c

CLEANFILES = $(BUILT_SOURCES)

@wayland_scanner_rules@
