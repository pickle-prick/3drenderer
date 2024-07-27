build_debug:
	mkdir -p ./build/debug && gcc -Wall -Wno-comment -std=c99 -g ./src/*.c -lSDL2 -lm -o ./build/debug/renderer
debug:
	make clean
	make build_debug
	gdb ./build/debug/renderer
build:
	mkdir -p ./build/release && gcc -Wall -Wno-comment -std=c99 ./src/*.c -lSDL2 -lm -o ./build/release/renderer
clean:
	rm -rf ./build
run:
	make clean
	make build
	./build/release/renderer
