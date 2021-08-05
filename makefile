OPT=$(shell pkg-config --libs --cflags gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 ) -lm -w -g -no-pie

all:imganywhere ui/*.glade ui/*.xml

imganywhere:*.c ui/*.c
	-rm ui/gresources.c ui/gresources.h
	make -C ui
	gcc $^ -o $@ $(OPT)
clean:
	-rm *.png *.bin imganywhere
