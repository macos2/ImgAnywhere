OPT=$(shell pkg-config --libs --cflags gtk+-3.0) -lm -w -g

all:imganywhere.exe

imganywhere.exe:main.c  ImgTool.c ImgFormat.c SurfFormat.c ui/MyMain.c ui/MyVideoArea.c
	gcc $^ -o $@ $(OPT)

clean:
	-rm *.png *.bin imganywhere.exe
