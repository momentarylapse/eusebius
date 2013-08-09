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

kernel2.o : kernel2.kaba
	$(KABA) --x86 -o kernel2.o kernel2.kaba
 
loader_fake.o : loader_fake.kaba
	$(KABA) --x86 -o loader_fake.o loader_fake.kaba
 
loader_br.o : loader_br.kaba
	$(KABA) --x86 -o loader_br.o loader_br.kaba

bochs/c0.img:
	dd if=/dev/zero of=bochs/c0.img bs=1024 count=10080

Programme/hello.o: Programme/hello.kaba
	$(KABA) --x86 -o Programme/hello.o Programme/hello.kaba

Programme/shell.o: Programme/shell.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o Programme/shell.o Programme/shell.kaba

Programme/kalib.o: Programme/kalib.kaba
	$(KABA) --x86 -o Programme/kalib.o --export-symbols kalib_symbols Programme/kalib.kaba

kalib_symbols : Programme/kalib.o

img.mfs: init.o kernel2.o Programme/hello.o Programme/shell.o Programme/kalib.o
	cp init.o mfs/000-init
	cp kernel2.o mfs/001-kernel
	cp Programme/hello.o mfs/hello
	cp Programme/shell.o mfs/shell
	cp Programme/kalib.o mfs/kalib
	$(MAKEMFS) `pwd`/img.mfs `pwd`/mfs/

bochs/c.img: bochs/c0.img img.mfs loader_fake.o
	cp bochs/c0.img bochs/c.img
	dd if=bochs/mbr0 of=bochs/c.img conv=notrunc
	dd if=loader_fake.o of=bochs/c.img conv=notrunc
	dd if=img.mfs of=bochs/c.img bs=512 seek=16 conv=notrunc

clean:
	rm -f *.o Programme/*.o bochs/c.img img.mfs

