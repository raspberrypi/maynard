gsettings_SCHEMAS = org.raspberrypi.maynard.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@

%.gschema.xml.in: %.gschema.xml.in.in Makefile
	$(AM_V_GEN) sed -e 's|@GETTEXT_PACKAGE[@]|$(GETTEXT_PACKAGE)|g' \
	$< > $@ || rm $@

@GSETTINGS_RULES@

# We need to compile schemas at make time
# to run from source tree
gschemas.compiled: $(gsettings_SCHEMAS:.xml=.valid)
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) --targetdir=. .

all-local: gschemas.compiled

EXTRA_DIST =						\
	org.raspberrypi.maynard.gschema.xml.in.in

CLEANFILES =						\
	gschemas.compiled				\
	org.raspberrypi.maynard.gschema.valid	\
	$(gsettings_SCHEMAS)				\
	$(gsettings_SCHEMAS).in
