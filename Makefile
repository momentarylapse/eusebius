KABA  = kaba
#KABA  = ~/Projekte/Kaba/Debug/Kaba
FLAGS =  --x86 --no-std-lib
PFLAGS =  --x86 --no-std-lib --import-symbols kalib_symbols
#MAKEMFS = ./tools/makemfs/makemfs
MAKEMFS = $(KABA) tools/makemfs.kaba

all : bochs/c.img

test.o : test.kaba
	$(KABA) --x86 -o test.o test.kaba

init.o : init.kaba
	$(KABA) --x86 -o init.o init.kaba

kernel/kernel.o : kernel/kernel.kaba kernel/base.kaba
	$(KABA) --x86 -o kernel/kernel.o kernel/kernel.kaba
 
loader_fake.o : loader_fake.kaba
	$(KABA) --x86 -o loader_fake.o loader_fake.kaba
 
loader_br.o : loader_br.kaba
	$(KABA) --x86 -o loader_br.o loader_br.kaba

Programme/hello.o: Programme/hello.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/hello.o Programme/hello.kaba

Programme/shell.o: Programme/shell.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/shell.o Programme/shell.kaba

Programme/cat.o: Programme/cat.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/cat.o Programme/cat.kaba

Programme/hd.o: Programme/hd.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/hd.o Programme/hd.kaba

Programme/echo.o: Programme/echo.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/echo.o Programme/echo.kaba

Programme/kill.o: Programme/kill.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/kill.o Programme/kill.kaba

Programme/ls.o: Programme/ls.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/ls.o Programme/ls.kaba

Programme/top.o: Programme/top.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/top.o Programme/top.kaba

Programme/kalib.o: Programme/kalib.kaba
	$(KABA) --x86 -o Programme/kalib.o --export-symbols kalib_symbols Programme/kalib.kaba

kalib_symbols : Programme/kalib.o

img.mfs: init.o kernel/kernel.o Programme/hello.o Programme/shell.o Programme/cat.o Programme/echo.o Programme/kill.o Programme/top.o Programme/ls.o Programme/hd.o Programme/kalib.o
	cp init.o mfs/000-init
	cp kernel/kernel.o mfs/001-kernel
	cp Programme/hello.o mfs/hello
	cp Programme/shell.o mfs/shell
	cp Programme/cat.o mfs/cat
	cp Programme/hd.o mfs/hd
	cp Programme/ls.o mfs/ls
	cp Programme/top.o mfs/top
	cp Programme/echo.o mfs/echo
	cp Programme/kill.o mfs/kill
	cp Programme/kalib.o mfs/kalib
	$(MAKEMFS) `pwd`/img.mfs `pwd`/mfs/

bochs/c.img: img.mfs loader_fake.o
	dd if=/dev/zero of=bochs/c.img bs=1024 count=20160
	if [ -f Experimente/ext2/img2 ]; \
	then \
	    dd if=bochs/mbr0_ext2 of=bochs/c.img conv=notrunc; \
	    dd if=Experimente/ext2/img2 of=bochs/c.img bs=1024 seek=10080 conv=notrunc; \
	else \
	    dd if=bochs/mbr0 of=bochs/c.img conv=notrunc; \
	fi
	dd if=loader_fake.o of=bochs/c.img conv=notrunc
	dd if=img.mfs of=bochs/c.img bs=512 seek=16 conv=notrunc

clean:
	rm -f *.o kernel/*.o Programme/*.o bochs/c.img img.mfs

