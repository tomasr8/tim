

all:
	clang++ -std=c++17 tim.cpp `pkg-config --libs --cflags opencv4` -o tim

clean:
	rm tim