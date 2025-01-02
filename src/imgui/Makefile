build:
	rm -f ../../libimgui.a
	g++ -DNO_FONT_AWESOME -c bridge.cpp -g
	ar rcs ../../libimgui.a *.o

all:
	rm -f ../../libimgui.a
	g++ -DNO_FONT_AWESOME -c *.cpp -g
	ar rcs ../../libimgui.a *.o
