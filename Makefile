KABA  = kaba
#KABA  = ~/Projekte/Kaba/Debug/Kaba
MACHINE = --x86
FLAGS =  $(MACHINE) --no-std-lib
LOADERFLAGS = $(MACHINE) --os --no-function-frames --code-origin 0x7c00
INITFLAGS = $(MACHINE) --os --no-function-frames --code-origin 0x7e00
KERNELFLAGS = $(MACHINE)  --os --no-std-lib --code-origin 0x00010000 --add-entry-point --variable-offset 0x00100000 --no-std-lib
PFLAGS =  $(MACHINE) --os --no-std-lib --code-origin 0x00800000 --variable-offset 0x00880000 --add-entry-point --import-symbols kalib_symbols
LIBFLAGS = $(MACHINE) --no-std-lib --os --no-std-lib --code-origin 0x00050000 --variable-offset 0x00a3f000
#MAKEMFS = ./tools/makemfs/makemfs
MAKEMFS = $(KABA) tools/makemfs.kaba
BINS = bin/hello bin/shell bin/cat bin/cmp bin/echo bin/kill bin/top bin/ls bin/hd bin/touch bin/mkdir bin/tr bin/mkfifo bin/less bin/x bin/shmem bin/date bin/sleep bin/uname bin/client bin/pci bin/net bin/error bin/k bin/rm bin/rmdir bin/cake bin/c bin/xterm bin/xtest bin/xedit bin/ximage bin/xfiles bin/xdesktop
PDEP = lib/kalib.kaba kalib_symbols bin/lib/*.kaba bin/lib/*/*.kaba
LIBS = lib/kalib

all : bochs/c.img

init : init.kaba
	$(KABA) $(INITFLAGS) -o init init.kaba

kernel/kernel : kernel/*.kaba kernel/dev/*.kaba kernel/fs/*.kaba kernel/io/*.kaba kernel/irq/*.kaba kernel/mem/*.kaba kernel/task/*.kaba kernel/time/*.kaba kernel/net/*.kaba
	$(KABA) $(KERNELFLAGS) -o kernel/kernel kernel/kernel.kaba
 
loader_fake : loader_fake.kaba
	$(KABA) $(LOADERFLAGS) -o loader_fake loader_fake.kaba
 
loader_br : loader_br.kaba
	$(KABA) $(LOADERFLAGS) -o loader_br loader_br.kaba

bin/hello: bin/hello.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/hello bin/hello.kaba

bin/shell: bin/shell.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/shell bin/shell.kaba

bin/cat: bin/cat.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/cat bin/cat.kaba

bin/cmp: bin/cmp.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/cmp bin/cmp.kaba

bin/hd: bin/hd.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/hd bin/hd.kaba

bin/echo: bin/echo.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/echo bin/echo.kaba

bin/kill: bin/kill.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/kill bin/kill.kaba

bin/ls: bin/ls.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/ls bin/ls.kaba

bin/top: bin/top.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/top bin/top.kaba

bin/touch: bin/touch.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/touch bin/touch.kaba

bin/mkdir: bin/mkdir.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/mkdir bin/mkdir.kaba

bin/mkfifo: bin/mkfifo.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/mkfifo bin/mkfifo.kaba

bin/tr: bin/tr.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/tr bin/tr.kaba

bin/less: bin/less.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/less bin/less.kaba

bin/x: bin/x.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/x bin/x.kaba

bin/xterm: bin/xterm.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/xterm bin/xterm.kaba

bin/xtest: bin/xtest.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/xtest bin/xtest.kaba

bin/xedit: bin/xedit.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/xedit bin/xedit.kaba

bin/ximage: bin/ximage.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/ximage bin/ximage.kaba

bin/xfiles: bin/xfiles.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/xfiles bin/xfiles.kaba

bin/xdesktop: bin/xdesktop.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/xdesktop bin/xdesktop.kaba

bin/shmem: bin/shmem.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/shmem bin/shmem.kaba

bin/date: bin/date.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/date bin/date.kaba

bin/sleep: bin/sleep.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/sleep bin/sleep.kaba

bin/uname: bin/uname.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/uname bin/uname.kaba

bin/client: bin/client.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/client bin/client.kaba

bin/pci: bin/pci.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/pci bin/pci.kaba

bin/net: bin/net.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/net bin/net.kaba

bin/rm: bin/rm.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/rm bin/rm.kaba

bin/rmdir: bin/rmdir.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/rmdir bin/rmdir.kaba

bin/error: bin/error.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/error bin/error.kaba

bin/k: bin/k.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/k bin/k.kaba

bin/c: bin/c.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/c bin/c.kaba

bin/cake: bin/cake.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/cake bin/cake.kaba

lib/kalib: lib/kalib.kaba
	$(KABA) $(LIBFLAGS) -o lib/kalib --export-symbols kalib_symbols lib/kalib.kaba

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
	cp -r home/* img-src/home
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

