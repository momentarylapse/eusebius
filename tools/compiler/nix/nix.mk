# module: nix

NIX_BIN  = temp/nix.a
NIX_OBJ  = temp/nix.o temp/nix_types.o temp/nix_textures.o temp/nix_net.o temp/nix_sound.o
NIX_CXXFLAGS =  `pkg-config --cflags gtk+-2.0` -Wno-write-strings -O2

$(NIX_BIN) : $(NIX_OBJ)
	rm -f $@
	ar cq $@ $(NIX_OBJ)

temp/nix.o : nix/nix.cpp
	$(CPP) -c nix/nix.cpp -o $@ $(NIX_CXXFLAGS)

temp/nix_types.o : nix/nix_types.cpp
	$(CPP) -c nix/nix_types.cpp -o $@ $(NIX_CXXFLAGS)

temp/nix_sound.o : nix/nix_sound.cpp
	$(CPP) -c nix/nix_sound.cpp -o $@ $(NIX_CXXFLAGS)

temp/nix_textures.o : nix/nix_textures.cpp
	$(CPP) -c nix/nix_textures.cpp -o $@ $(NIX_CXXFLAGS)

temp/nix_net.o : nix/nix_net.cpp
	$(CPP) -c nix/nix_net.cpp -o $@ $(NIX_CXXFLAGS)

nix/nix.cpp : nix/nix.h
nix/nix_types.cpp : nix/nix.h
nix/nix_sound.cpp : nix/nix.h
nix/nix_textures.cpp : nix/nix.h
nix/nix_net.cpp : nix/nix.h
nix/nix.h : 00_config.h nix/nix_config.h nix/nix_textures.h nix/nix_types.h nix/nix_sound.h nix/nix_net.h
nix/nix_config.h : 00_config.h hui/hui.h file/msg.h
#nix/nix_net.h : nix/nix.h

