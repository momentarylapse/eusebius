KABA  = ./tools/compiler/compiler
#KABA  = ./compiler
MAKEMFS = ./tools/makemfs/makemfs

all : loader_br init kernel image.mfs hd_image _bochs_

loader_mbr : loader_mbr.kaba
	$(KABA) -o loader_mbr.kaba loader_mbr

loader_br : loader_br.kaba
	$(KABA) -o loader_br.kaba loader_br

init : init.kaba
	$(KABA) -o init.kaba init

kernel : kernel.kaba
	$(KABA) -o kernel.kaba kernel

image.mfs : init kernel
	cp init mfs/000_init
	cp kernel mfs/001_kernel
	$(MAKEMFS) image.mfs mfs

hd_image : image.mfs loader_br
	dd if=br_empty of=hd_image
	dd if=loader_br of=hd_image bs=512 conv=notrunc
	dd if=image.mfs of=hd_image bs=512 seek=1

_bochs_ : bochs/hd10meg.img

bochs/hd10meg.img : hd_image
	dd if=hd_image of=bochs/hd10meg.img bs=512 seek=17 conv=notrunc

_real_ : hd_image
	dd if=hd_image of=/dev/sda4

clean :
	rm -f loader_br loader_mbr init kernel image.mfs hd_image
