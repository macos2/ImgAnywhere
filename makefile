OPT=$(shell pkg-config --libs --cflags gtk+-3.0) -lm -w -g

all:imganywhere

imganywhere:main.c MyVideoArea.c ImgTool.c
	gcc $^ -o $@ $(OPT)

clean:
	-rm *.png imganywhere
