OPT=$(shell pkg-config --libs --cflags gtk+-3.0) -lm -w -g

all:imganywhere.exe

imganywhere.exe:main.c MyVideoArea.c ImgTool.c ImgFormat.c SurfFormat.c
	gcc $^ -o $@ $(OPT)

clean:
	-rm *.png *.bin imganywhere.exe
