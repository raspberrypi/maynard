libexec_PROGRAMS = maynard

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = $(CLIENT_CFLAGS) $(GTK_CFLAGS)

maynard_SOURCES =				\
	maynard.c				\
	app-icon.c				\
	app-icon.h				\
	clock.c					\
	clock.h					\
	favorites.c				\
	favorites.h				\
	shell-app-system.c			\
	shell-app-system.h			\
	panel.c					\
	panel.h					\
	vertical-clock.c			\
	vertical-clock.h			\
	launcher.c				\
	launcher.h				\
	maynard-resources.c			\
	maynard-resources.h			\
	weston-desktop-shell-client-protocol.h	\
	weston-desktop-shell-protocol.c		\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c		\
	shell-helper-client-protocol.h		\
	shell-helper-protocol.c
maynard_LDADD = $(GTK_LIBS) -lm

BUILT_SOURCES =					\
	weston-desktop-shell-client-protocol.h	\
	weston-desktop-shell-protocol.c		\
	desktop-shell-client-protocol.h		\
	desktop-shell-protocol.c		\
	shell-helper-client-protocol.h		\
	shell-helper-protocol.c			\
	maynard-resources.c			\
	maynard-resources.h

# gresource for css
resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/maynard.gresource.xml)
maynard-resources.c: maynard.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name maynard $<
maynard-resources.h: maynard.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name maynard $<

EXTRA_DIST =			\
	style.css		\
	maynard.gresource.xml

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
