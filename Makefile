cpp:
	g++ -g -O0 -o j4on j4on.cpp test.cpp -std=c++17

run:
	./j4on

clean:
	rm -rf a.out a.exe j4on