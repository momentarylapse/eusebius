all : compiler

compiler : compiler.cpp dasm.o file.o msg.o script.o script_data.o nix.o hui.o
	g++ compiler.cpp dasm.o file.o msg.o script.o script_data.o nix.o hui.o -o compiler `pkg-config --cflags --libs gtk+-2.0` -I/opt/gnome/include -lGL -lgdk -lgtk -lGLU -L/usr/X11R6/lib -lXxf86vm

nix.o : nix.cpp nix.h msg.o
	g++ -c nix.cpp `pkg-config --cflags --libs gtk+-2.0` -I/opt/gnome/include -lGL -lgdk -lgtk -lGLU -L/usr/X11R6/lib -lXxf86vm

hui.o : hui.cpp hui.h msg.o
	g++ -c hui.cpp `pkg-config --cflags --libs gtk+-2.0` -I/opt/gnome/include -lGL -lgdk -lgtk -lGLU -L/usr/X11R6/lib -lXxf86vm

file.o : file.cpp file.h
	g++ -c file.cpp

msg.o : msg.cpp msg.h file.o
	g++ -c msg.cpp

dasm.o : dasm.cpp dasm.h
	g++ -c dasm.cpp

script.o : script.cpp script.h script_data.h msg.o dasm.o
	g++ -c script.cpp

script_data.o : script_data.cpp script_data.h script.h msg.o
	g++ -c script_data.cpp `pkg-config --cflags --libs gtk+-2.0` -I/opt/gnome/include -lGL -lgdk -lgtk -lGLU -L/usr/X11R6/lib -lXxf86vm

clean :
	\rm *.o *~ compiler
