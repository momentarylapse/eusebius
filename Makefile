KABA  = kaba
#KABA  = ~/Projekte/Kaba/Debug/Kaba
MAKEMFS = ./tools/makemfs/makemfs

all : bochs/c.img

test.o : test.kaba
	$(KABA) --x86 -o test.o test.kaba
 
loader_fake.o : loader_fake.kaba
	$(KABA) --x86 -o loader_fake.o loader_fake.kaba 

bochs/c.img: loader_fake.o test.o
	#dd if=test.o of=bochs/c.img conv=notrunc
	dd if=loader_fake.o of=bochs/c.img conv=notrunc
	dd if=test.o of=bochs/c.img bs=512 seek=1 conv=notrunc

clean:
	rm -f *.o

