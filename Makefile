build:
	gcc -o out/nes -g src/*.c src/mappers/*.c -lraylib -L. -lm -lSDL2 -limgui -lstdc++
	./out/nes
