OPT=$(shell pkg-config --libs --cflags gtk+-3.0 gstreamer-1.0) -lm -w -g -no-pie#-g 

all:imganywhere ui/*.glade ui/*.xml *.c ui/*.c

imganywhere:*.c ui/*.c
	-rm ui/gresources.c ui/gresources.h
	make -C ui
	gcc $^ -o $@ $(OPT)
clean:
	-rm *.png *.bin imganywhere
