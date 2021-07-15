OPT=$(shell pkg-config --libs --cflags gtk+-3.0 gstreamer-1.0 ) -lm -w -g -no-pie

all:imganywhere

imganywhere:*.c ui/*.c
	gcc $^ -o $@ $(OPT)

clean:
	-rm *.png *.bin imganywhere
