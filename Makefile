rebase:
	mkdir bin

build:
	cmake -B . -S .. && cmake --build ..

clean:
	rm -rf bin
