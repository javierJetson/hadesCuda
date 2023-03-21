# Compilers
CC = aarch64-linux-gnu-gcc.exe
CXX = g++ # Will need to install raspberry64 
CFLAGS = -g -Werrors -Wfatal-errors
INCLUDE = -I"C:\Xilinx\Vitis_HLS\2022.2\include"

iznTester: iznTest.o
	$(CXX) ./iznTestCpu.o ./iznCpu.o

iznTest.o: IZN.o
	$(CXX) -O3 ./iznTest.cpp -c -o ./iznTestCpu.o $(INCLUDE) 

IZN.o:
	$(CXX) -O3 ./IZN.cpp -c -o iznCpu.o $(INCLUDE)
	