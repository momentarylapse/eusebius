# module: file

FILE_MODULE  = temp/file.a
FILE_OBJ  = temp/file.o temp/msg.o temp/file_op.o temp/array.o temp/strings.o
FILE_CXXFLAGS = $(GLOBALFLAGS)


$(FILE_MODULE) : $(FILE_OBJ)
	rm -f $@
	ar cq $@ $(FILE_OBJ)

temp/file.o : file/file.cpp
	$(CPP) -c file/file.cpp -o $@ $(FILE_CXXFLAGS)

temp/msg.o : file/msg.cpp
	$(CPP) -c file/msg.cpp -o $@ $(FILE_CXXFLAGS)

temp/file_op.o : file/file_op.cpp
	$(CPP) -c file/file_op.cpp -o $@ $(FILE_CXXFLAGS)

temp/array.o : file/array.cpp
	$(CPP) -c file/array.cpp -o $@ $(FILE_CXXFLAGS)

temp/strings.o : file/strings.cpp
	$(CPP) -c file/strings.cpp -o $@ $(FILE_CXXFLAGS)

#file/file.cpp : file/file.h file/msg.h
#file/msg.cpp : file/msg.h
#file/msg.h : file/file.h

