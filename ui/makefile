all:gresources.c gresources.h $(shell glib-compile-resources --generate-dependencies gresources.xml) gresources.xml

gresources.c:$(shell glib-compile-resources --generate-dependencies gresources.xml) gresources.xml
	glib-compile-resources --generate-source gresources.xml

gresources.h:$(shell glib-compile-resources --generate-dependencies gresources.xml) gresources.xml
	glib-compile-resources --generate-header gresources.xml
