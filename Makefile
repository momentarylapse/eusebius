KABA  = kaba
#KABA  = ~/Projects/Kaba/kaba
#KABA  = ~/Projekte/Kaba/kaba --verbose
#KABA  = valgrind ~/Projekte/Kaba/kaba --verbose
MACHINE = --arch amd64:gnu
FLAGS =  $(MACHINE) --no-std-lib
LOADERFLAGS = --arch x86:gnu --code-origin 0x7c00
INITFLAGS = --arch x86:gnu --code-origin 0x7e00
KERNELFLAGS = $(MACHINE) --no-std-lib --code-origin 0x00010000 --add-entry-point --variable-offset 0x00100000 --no-std-lib
# --remove-unused
PFLAGS =  $(MACHINE) --no-std-lib --code-origin 0x40000000 --variable-offset 0x40080000 --add-entry-point --import-symbols kalib_symbols

LIBFLAGS = $(MACHINE) --no-std-lib --no-std-lib --code-origin 0x00050000 --variable-offset 0x00a3f000
#MAKEMFS = ./tools/makemfs/makemfs
MAKEMFS = $(KABA) tools/makemfs.kaba
BINS = \
 bin/cat \
 bin/cpuid \
 bin/date \
 bin/echo \
 bin/error \
 bin/hd \
 bin/kill \
 bin/ls \
 bin/lsblk \
 bin/lspci \
 bin/mkdir \
 bin/mkfifo \
 bin/mount \
 bin/pwd \
 bin/rm \
 bin/rmdir \
 bin/shell \
 bin/shmem \
 bin/simple \
 bin/sleep \
 bin/sock \
 bin/top \
 bin/touch \
 bin/uname \
 bin/usb \
 bin/vt \
 bin/x \
 bin/xcake \
 bin/xclient \
 bin/xclient2
 
XXXBINS = \
 bin/hello \
 bin/cmp \
 bin/tr \
 bin/less \
 bin/net bin/sound \
 bin/k \
 bin/xterm \
 bin/xtest \
 bin/xedit \
 bin/ximage \
 bin/xfiles \
 bin/xdesktop
PDEP = kalib_symbols lib/*.kaba bin/lib/std/*.kaba
PDEPX = $(PDEP) bin/lib/x/x.kaba bin/lib/ttfx.kaba bin/lib/draw.kaba
LIBS = lib/kalib

all : bochs/c.img

test : bochs/c-test.img

init : init.kaba
	$(KABA) $(INITFLAGS) -o init init.kaba
	echo "HALLO" >> init

kernel/kernel : kernel/*.kaba kernel/*/*.kaba
	$(KABA) $(KERNELFLAGS) -o kernel/kernel kernel/main.kaba

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

bin/cpuid: bin/cpuid.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/cpuid bin/cpuid.kaba

bin/hd: bin/hd.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/hd bin/hd.kaba

bin/echo: bin/echo.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/echo bin/echo.kaba

bin/kill: bin/kill.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/kill bin/kill.kaba

bin/less: bin/less.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/less bin/less.kaba

bin/ls: bin/ls.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/ls bin/ls.kaba

bin/lsblk: bin/lsblk.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/lsblk bin/lsblk.kaba

bin/mkdir: bin/mkdir.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/mkdir bin/mkdir.kaba

bin/mkfifo: bin/mkfifo.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/mkfifo bin/mkfifo.kaba

bin/mount: bin/mount.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/mount bin/mount.kaba

bin/pwd: bin/pwd.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/pwd bin/pwd.kaba

bin/top: bin/top.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/top bin/top.kaba

bin/touch: bin/touch.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/touch bin/touch.kaba

bin/tr: bin/tr.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/tr bin/tr.kaba

bin/usb: bin/usb.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/usb bin/usb.kaba

bin/x: bin/x.kaba $(PDEP) bin/lib/xserver/*.kaba bin/lib/vesa.kaba bin/lib/pci.kaba
	$(KABA) $(PFLAGS) -o bin/x bin/x.kaba

bin/xterm: bin/xterm.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xterm bin/xterm.kaba

bin/xtest: bin/xtest.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xtest bin/xtest.kaba

bin/xedit: bin/xedit.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xedit bin/xedit.kaba

bin/ximage: bin/ximage.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/ximage bin/ximage.kaba

bin/xfiles: bin/xfiles.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xfiles bin/xfiles.kaba

bin/xdesktop: bin/xdesktop.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xdesktop bin/xdesktop.kaba

bin/shmem: bin/shmem.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/shmem bin/shmem.kaba

bin/date: bin/date.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/date bin/date.kaba

bin/sleep: bin/sleep.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/sleep bin/sleep.kaba

bin/uname: bin/uname.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/uname bin/uname.kaba

bin/xclient: bin/xclient.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xclient bin/xclient.kaba

bin/lspci: bin/lspci.kaba $(PDEP) bin/lib/pci.kaba
	$(KABA) $(PFLAGS) -o bin/lspci bin/lspci.kaba

bin/net: bin/net.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/net bin/net.kaba

bin/sound: bin/sound.kaba $(PDEP) bin/lib/pci.kaba
	$(KABA) $(PFLAGS) -o bin/sound bin/sound.kaba

bin/sock: bin/sock.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/sock bin/sock.kaba

bin/rm: bin/rm.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/rm bin/rm.kaba

bin/rmdir: bin/rmdir.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/rmdir bin/rmdir.kaba

bin/error: bin/error.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/error bin/error.kaba

bin/k: bin/k.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/k bin/k.kaba

bin/xclient2: bin/xclient2.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xclient2 bin/xclient2.kaba

bin/xcake: bin/xcake.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/xcake bin/xcake.kaba

bin/simple: bin/simple.kaba $(PDEP)
	$(KABA) $(PFLAGS) -o bin/simple bin/simple.kaba

bin/vt: bin/vt.kaba $(PDEPX)
	$(KABA) $(PFLAGS) -o bin/vt bin/vt.kaba

lib/kalib: lib/kalib.kaba lib/lib_*.kaba
	$(KABA) $(LIBFLAGS) -o lib/kalib --export-symbols kalib_symbols lib/kalib.kaba
	
kalib_symbols : lib/kalib lib/kalib.kaba

#img.mfs: init kernel/kernel $(BINS)
img.mfs: init kernel/kernel
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
	# block-size 1k, 256 inodes, size 4M
	#genext2fs -b 4096 -N 256 -d img-src img.ext2
	mkfs.ext2 -b 1024 -N 256 -Fq -d img-src img.ext2 4M
	#mkfs.ext2 -b 2048 -N 256 -Fq -d img-src img.ext2 8M

bochs/c.img: img.mfs img.ext2 loader_fake
	dd if=/dev/zero of=bochs/c.img bs=1024 count=20160
	dd if=bochs/mbr0_ext2 of=bochs/c.img conv=notrunc
	dd if=loader_fake of=bochs/c.img conv=notrunc
	dd if=img.mfs of=bochs/c.img bs=1024 seek=8 conv=notrunc
	# ext2 @ 0x9d8000
	dd if=img.ext2 of=bochs/c.img bs=1024 seek=10080 conv=notrunc
	


img-test.ext2: img.mfs
	mkdir -p img-src
	mkdir -p img-src/dev
	mkdir -p img-src/boot
	mkdir -p img-src/home
	mkdir -p img-src/images
	mkdir -p img-src/tmp
	# block-size 1k, 256 inodes, size 4M
	#genext2fs -b 4096 -N 256 -d img-src img.ext2
	mkfs.ext2 -b 1024 -N 256 -Fq -d img-src img.ext2 4M
	#mkfs.ext2 -b 2048 -N 256 -Fq -d img-src img.ext2 8M

bochs/c-test.img: img.mfs img-test.ext2 loader_fake
	dd if=/dev/zero of=bochs/c.img bs=1024 count=20160
	dd if=bochs/mbr0_ext2 of=bochs/c.img conv=notrunc
	dd if=loader_fake of=bochs/c.img conv=notrunc
	dd if=img.mfs of=bochs/c.img bs=1024 seek=8 conv=notrunc
	# ext2 @ 0x9d8000
	dd if=img.ext2 of=bochs/c.img bs=1024 seek=10080 conv=notrunc

clean:
	rm -rf init loader_fake kernel/kernel $(BINS) $(LIBS) bochs/c.img img.mfs img.ext2 img-src mfs

