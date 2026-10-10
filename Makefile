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

run-resolver:
	bin/dns-server --resolver 0.0.0.0:2054

test:
	bin/test-runner > test.log