# module: hui

HUI_BIN  = temp/hui.a
HUI_OBJ  = temp/hui.o temp/hui_menu.o temp/hui_window.o
HUI_CXXFLAGS =  `pkg-config --cflags gtk+-2.0` $(GLOBALFLAGS)


$(HUI_BIN) : $(HUI_OBJ)
	rm -f $@
	ar cq $@ $(HUI_OBJ)

temp/hui.o : hui/hui.cpp
	$(CPP) -c hui/hui.cpp -o $@ $(HUI_CXXFLAGS)

temp/hui_menu.o : hui/hui_menu.cpp
	$(CPP) -c hui/hui_menu.cpp -o $@ $(HUI_CXXFLAGS)

temp/hui_window.o : hui/hui_window.cpp
	$(CPP) -c hui/hui_window.cpp -o $@ $(HUI_CXXFLAGS)

hui/hui.cpp : hui/hui.h file/file.h
hui/hui_menu.cpp : hui/hui.h file/file.h
hui/hui_window.cpp : hui/hui.h file/file.h
hui/hui.h : hui/hui_config.h hui/hui_menu.h hui/hui_window.h
hui/hui_window.h : hui/hui_config.h

