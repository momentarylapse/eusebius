all : disasm

disasm : disasm.cpp dasm.o file.o msg.o
	g++ disasm.cpp dasm.o file.o msg.o -o disasm

file.o : file.cpp file.h
	g++ -c file.cpp

msg.o : msg.cpp msg.h file.o
	g++ -c msg.cpp

dasm.o : dasm.cpp dasm.h
	g++ -c dasm.cpp

clean :
	\rm *.o *~ disasm
