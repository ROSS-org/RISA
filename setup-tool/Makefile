CXX=g++
CXXFLAGS=-std=c++11 -Wno-deprecated-declarations -g -I/home/rossc3/local/include

setupmake: setup.o Model.o
	$(CXX) -o setup setup.o Model.o -L/home/rossc3/local/lib -lxerces-c

