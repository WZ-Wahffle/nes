build:
	gcc -g -o out/nes src/*.c src/mappers/*.c -lraylib -L. -lm -lSDL2 -limgui -lstdc++
	./out/nes
