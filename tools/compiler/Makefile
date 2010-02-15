# Project: Compiler

BIN  = compiler
MAINSOURCE = compiler.cpp
MODULES = temp/file.a temp/hui.a temp/nix.a temp/script.a
OBJ = temp/file.o temp/msg.o \
temp/hui.o temp/hui_menu.o temp/hui_window.o \
temp/nix.o temp/nix_types.o temp/nix_textures.o temp/nix_net.o temp/nix_sound.o \
temp/script.o temp/script_data.o temp/dasm.o
CPP  = g++
CXXFLAGS =  `pkg-config --cflags gtk+-2.0` -Wno-write-strings -O1
CXXFLAGSLINKING =  `pkg-config --cflags --libs gtk+-2.0` -lGL -lGLU -lXxf86vm -Wno-write-strings -O1

all : $(BIN)

$(BIN) : $(MAINSOURCE) $(OBJ) $(MODULES)
	$(CPP) $(OBJ) $(MODULES) $(MAINSOURCE) -o $(BIN) $(CXXFLAGSLINKING)

include file/file.mk
include hui/hui.mk
include nix/nix.mk
include script/script.mk

clean :
	#rm -f $(OBJ) $(BIN)
	rm -f temp/* $(BIN)