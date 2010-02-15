# module: file, msg

FILE_MODULE  = temp/file.a
FILE_OBJ  = temp/file.o temp/msg.o
FILE_CXXFLAGS =  -Wno-write-strings -O2


$(FILE_MODULE) : $(FILE_OBJ)
	rm -f $@
	ar cq $@ $(FILE_OBJ)

temp/file.o : file/file.cpp
	$(CPP) -c file/file.cpp -o $@ $(FILE_CXXFLAGS)

temp/msg.o : file/msg.cpp
	$(CPP) -c file/msg.cpp -o $@ $(FILE_CXXFLAGS)

file/file.cpp : file/file.h file/msg.h
file/msg.cpp : file/msg.h
file/msg.h : file/file.h

