rebase:
	mkdir bin

build:
	cd bin &&	\
	cmake -B . -S .. && \
	cmake --build .

clean:
	rm -rf bin/*

run:
	bin/dns-server

test:
	bin/test-runner > test.log