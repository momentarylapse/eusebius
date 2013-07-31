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

bochs/c.img: loader_fake.o init.o kernel2.o
	#dd if=test.o of=bochs/c.img conv=notrunc
	dd if=loader_fake.o of=bochs/c.img conv=notrunc
	dd if=init.o of=bochs/c.img bs=512 seek=1 conv=notrunc
	#dd if=test.o of=bochs/c.img bs=512 seek=1 conv=notrunc
	dd if=kernel2.o of=bochs/c.img bs=512 seek=2 conv=notrunc

clean:
	rm -f *.o

