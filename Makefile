CPPFLAGS = -O2 -W -Wall --pedantic

zg01_test: zg01_test.cpp zg01/zg01_fsm.cpp

clean:
	rm -f zg01_test

test: zg01_test
	./zg01_test

