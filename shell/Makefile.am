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
	favorites.c				\
	favorites.h				\
	launcher-grid.c				\
	launcher-grid.h				\
	shell-app-system.c			\
	shell-app-system.h			\
	sound-applet.c				\
	sound-applet.h				\
	weston-gtk-shell-resources.c		\
	weston-gtk-shell-resources.h		\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c		\
	$(external_sources)
weston_gtk_shell_LDADD = $(GTK_LIBS)

BUILT_SOURCES =					\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c		\
	weston-gtk-shell-resources.c		\
	weston-gtk-shell-resources.h

# gresource for css
resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/weston-gtk-shell.gresource.xml)
weston-gtk-shell-resources.c: weston-gtk-shell.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name weston_gtk_shell $<
weston-gtk-shell-resources.h: weston-gtk-shell.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name weston_gtk_shell $<

EXTRA_DIST = style.css

CLEANFILES = $(BUILT_SOURCES)

@wayland_scanner_rules@
