# module: script

SCRIPT_BIN  = temp/script.a
SCRIPT_OBJ  = temp/script.o temp/pre_script.o temp/script_data.o temp/dasm.o
SCRIPT_CXXFLAGS =  `pkg-config --cflags gtk+-2.0` -Wno-write-strings -O2

$(SCRIPT_BIN) : $(SCRIPT_OBJ)
	rm -f $@
	ar cq $@ $(SCRIPT_OBJ)

temp/script.o : script/script.cpp
	$(CPP) -c script/script.cpp -o $@ $(SCRIPT_CXXFLAGS)

temp/pre_script.o : script/pre_script.cpp
	$(CPP) -c script/pre_script.cpp -o $@ $(SCRIPT_CXXFLAGS)

temp/script_data.o : script/script_data.cpp
	$(CPP) -c script/script_data.cpp -o $@ $(SCRIPT_CXXFLAGS)

temp/dasm.o : script/dasm.cpp
	$(CPP) -c script/dasm.cpp -o $@ $(SCRIPT_CXXFLAGS)

#nix/nix.cpp : nix/nix.h
#nix/nix_types.cpp : nix/nix.h
#nix/nix_sound.cpp : nix/nix.h
#nix/nix_textures.cpp : nix/nix.h
#nix/nix_net.cpp : nix/nix.h
#nix/nix.h : nix/00_config.h nix/nix_config.h nix/nix_textures.h nix/nix_types.h nix/nix_sound.h nix/nix_net.h
#nix/nix_config.h : nix/00_config.h hui/hui.h file/msg.h
#nix/nix_net.h : nix/nix.h

