KABA  = kaba
#KABA  = ~/Projekte/Kaba/Debug/Kaba
MAKEMFS = ./tools/makemfs/makemfs

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

Programme/shell.o: Programme/shell.kaba
	$(KABA) --x86 -o Programme/shell.o Programme/shell.kaba

img.mfs: init.o kernel2.o Programme/hello.o Programme/shell.o
	cp init.o mfs/000-init
	cp kernel2.o mfs/001-kernel
	cp Programme/hello.o mfs/hello
	cp Programme/shell.o mfs/shell
	$(KABA) tools/makemfs.kaba `pwd`/img.mfs `pwd`/mfs/

bochs/c.img: bochs/c0.img img.mfs loader_fake.o
	cp bochs/c0.img bochs/c.img
	dd if=bochs/mbr0 of=bochs/c.img conv=notrunc
	dd if=loader_fake.o of=bochs/c.img conv=notrunc
	dd if=img.mfs of=bochs/c.img bs=512 seek=16 conv=notrunc

clean:
	rm -f *.o bochs/c.img img.mfs

