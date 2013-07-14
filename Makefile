
MS_EXE=./bin/msvc-9.0/debug/threading-multi/demo.exe
GCC_EXE= bin\gcc-mingw-4.7.2\debug\threading-multi\demo.exe

TOOLSET=gcc

EXE=$(GCC_EXE)


$(EXE): demo.cpp
	P:/C/boost/boost_1_53_0/b2 

all:	$(EXE)

run:	$(EXE)
	$(EXE)



