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
	app-icon.c				\
	app-icon.h				\
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
	shell-helper-client-protocol.h		\
	shell-helper-protocol.c			\
	$(external_sources)
weston_gtk_shell_LDADD = $(GTK_LIBS)

BUILT_SOURCES =					\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c		\
	shell-helper-client-protocol.h		\
	shell-helper-protocol.c			\
	weston-gtk-shell-resources.c		\
	weston-gtk-shell-resources.h

# gresource for css
resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/weston-gtk-shell.gresource.xml)
weston-gtk-shell-resources.c: weston-gtk-shell.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name weston_gtk_shell $<
weston-gtk-shell-resources.h: weston-gtk-shell.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name weston_gtk_shell $<

EXTRA_DIST = style.css

# shell-helper weston module
moduledir = $(libdir)/weston
module_LTLIBRARIES = shell-helper.la

# otherwise shell-helper-protocol.c is generated twice- once with
# libtool and once without
mod-%-protocol.c : $(wayland_protocoldir)/%.xml
	$(AM_V_GEN)$(wayland_scanner) code < $< > $@

shell_helper_la_LDFLAGS = -module -avoid-version
shell_helper_la_LIBADD = $(GTK_LIBS)
shell_helper_la_SOURCES =				\
	shell-helper.c
nodist_shell_helper_la_SOURCES =			\
	mod-shell-helper-protocol.c 			\
	shell-helper-server-protocol.h

BUILT_SOURCES += $(nodist_shell_helper_la_SOURCES)
CLEANFILES = $(BUILT_SOURCES)

@wayland_scanner_rules@
