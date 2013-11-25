libexec_PROGRAMS = weston-gtk-shell

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = $(CLIENT_CFLAGS) $(GTK_CFLAGS)

external_sources =				\
	egg-flow-box-accessible.c		\
	egg-flow-box-accessible.h		\
	egg-flow-box.c				\
	egg-flow-box.h

weston_gtk_shell_SOURCES =			\
	gtk-shell.c				\
	app-launcher.c				\
	app-launcher.h				\
	clock.c					\
	clock.h					\
	launcher-grid.c				\
	launcher-grid.h				\
	shell-app-system.c			\
	shell-app-system.h			\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c		\
	$(external_sources)
weston_gtk_shell_LDADD = $(GTK_LIBS)

BUILT_SOURCES =					\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c

CLEANFILES = $(BUILT_SOURCES)

@wayland_scanner_rules@
