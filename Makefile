cpp:
	g++ -g -o j4on j4on.cc test.cc -std=c++17

run:
	./j4on

clean:
	rm -rf a.out a.exe j4on