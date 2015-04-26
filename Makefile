KABA  = kaba
#KABA  = ~/Projekte/Kaba/Debug/Kaba
FLAGS =  --x86 --no-std-lib
PFLAGS =  --x86 --no-std-lib --import-symbols kalib_symbols
#MAKEMFS = ./tools/makemfs/makemfs
MAKEMFS = $(KABA) tools/makemfs.kaba
BINS = bin/hello bin/shell bin/cat bin/echo bin/kill bin/top bin/ls bin/hd bin/touch bin/mkdir bin/tr bin/mkfifo bin/less bin/x bin/shmem bin/date bin/sleep bin/uname bin/client bin/pci bin/net bin/error
LIBS = lib/kalib

all : bochs/c.img

test : test.kaba
	$(KABA) --x86 -o test test.kaba

init : init.kaba
	$(KABA) --x86 -o init init.kaba

kernel/kernel : kernel/*.kaba kernel/mem/*.kaba kernel/task/*.kaba
	$(KABA) --x86 -o kernel/kernel kernel/kernel.kaba
 
loader_fake : loader_fake.kaba
	$(KABA) --x86 -o loader_fake loader_fake.kaba
 
loader_br : loader_br.kaba
	$(KABA) --x86 -o loader_br loader_br.kaba

bin/hello: bin/hello.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/hello bin/hello.kaba

bin/shell: bin/shell.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/shell bin/shell.kaba

bin/cat: bin/cat.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/cat bin/cat.kaba

bin/hd: bin/hd.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/hd bin/hd.kaba

bin/echo: bin/echo.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/echo bin/echo.kaba

bin/kill: bin/kill.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/kill bin/kill.kaba

bin/ls: bin/ls.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/ls bin/ls.kaba

bin/top: bin/top.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/top bin/top.kaba

bin/touch: bin/touch.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/touch bin/touch.kaba

bin/mkdir: bin/mkdir.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/mkdir bin/mkdir.kaba

bin/mkfifo: bin/mkfifo.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/mkfifo bin/mkfifo.kaba

bin/tr: bin/tr.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/tr bin/tr.kaba

bin/less: bin/less.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/less bin/less.kaba

bin/x: bin/x.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/x bin/x.kaba

bin/shmem: bin/shmem.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/shmem bin/shmem.kaba

bin/date: bin/date.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/date bin/date.kaba

bin/sleep: bin/sleep.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/sleep bin/sleep.kaba

bin/uname: bin/uname.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/uname bin/uname.kaba

bin/client: bin/client.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/client bin/client.kaba

bin/pci: bin/pci.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/pci bin/pci.kaba

bin/net: bin/net.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/net bin/net.kaba

bin/error: bin/error.kaba kalib_symbols
	$(KABA) $(PFLAGS) -o bin/error bin/error.kaba

lib/kalib: lib/kalib.kaba
	$(KABA) --x86 -o lib/kalib --export-symbols kalib_symbols lib/kalib.kaba

kalib_symbols : lib/kalib lib/kalib.kaba

img.mfs: init kernel/kernel $(BINS)
	mkdir -p mfs
	cp init mfs/000-init
	cp kernel/kernel mfs/kernel
	$(MAKEMFS) `pwd`/img.mfs `pwd`/mfs/

img.ext2: $(BINS) img.mfs
	mkdir -p img-src
	mkdir -p img-src/dev
	mkdir -p img-src/bin
	mkdir -p img-src/lib
	mkdir -p img-src/boot
	mkdir -p img-src/home
	mkdir -p img-src/src
	mkdir -p img-src/images
	mkdir -p img-src/tmp
	echo "aaa" > img-src/home/a
	echo "bbbb" > img-src/home/b
	echo "hallo\nkleiner Test" > img-src/home/test.txt
	cp -r $(BINS) img-src/bin
	cp -r $(LIBS) img-src/lib
	cp -r kernel/*.kaba img-src/src
	cp data/images/cursor.tga img-src/images
	genext2fs -b 4096 -N 256 -d img-src img.ext2 

bochs/c.img: img.mfs img.ext2 loader_fake
	dd if=/dev/zero of=bochs/c.img bs=1024 count=20160
	dd if=bochs/mbr0_ext2 of=bochs/c.img conv=notrunc
	dd if=loader_fake of=bochs/c.img conv=notrunc
	dd if=img.mfs of=bochs/c.img bs=1024 seek=8 conv=notrunc
	dd if=img.ext2 of=bochs/c.img bs=1024 seek=10080 conv=notrunc

run: all
	cd bochs; yes c | bochs -qf f.txt

clean:
	rm -rf init loader_fake kernel/kernel $(BINS) $(LIBS) bochs/c.img img.mfs img.ext2 img-src mfs

