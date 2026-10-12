rebase:
	mkdir bin

build:
	cd bin &&	\
	cmake -B . -S .. && \
	cmake --build .

clean:
	rm -rf bin/*

clean-logs:
	rm -rf logs/*

run:
	bin/dns-server

run-resolver:
	bin/dns-server --resolver 0.0.0.0:2054

run-as-resolver:
	bin/dns-server-resolver

test:
	bin/test-runner > test.log