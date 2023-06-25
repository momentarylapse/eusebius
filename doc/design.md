# Design

## Memory

### Address space

	----------------------------------- bios blocked (qemu)
	0x0009 fc00:0x000a 0000    1k
	0x000f 0000:0x0010 0000    64k
	0x1ffe 0000:0x2000 0000    128k
	0xfffc 0000:0xffff ffff    256k
	----------------------------------- loader
	0x0000 7c00:0x0000 7e00    loader code (512b)
	----------------------------------- init
	0x0000 0000:0x0180 0000    identity mapped (24m)
	0x0000 5100                mem size (4b)
	0x0000 5104                len(bio mem map) (4b)
	0x0000 6000:...            bios mem map (max 64 * 24b)
	0x0000 7e00:...            init code
	0x0010 0000:...            page tables
	----------------------------------- kernel space (id-mapped)
	0x0001 0000:0x0005 0000    kernel code (max 256k)
	0x0005 0000:...            kalib code
	0x000b 8000:...            video memory
	0x0010 0000:0x0080 0000    kernel variables
	  0x0028 0000:0x0070 0000    kernel heap (max 4.5m)
	  0x0070 0000:0x0080 0000    page tables
	0x00a3 f000:...            kalib variables
	----------------------------------- user space (randomly mapped)
	0x4000 0000:0x4007 0000    user task code (max 512k-64k)
	0x4007 0000:0x4008 0000    user task variables (64k)
	0x4008 0000:0x4030 0000    user task variables (2.5m)
	0x4030 0000:...            user task heap

### Paging

* init will identity-map first 24m (tables located @16m)
* kernel will identity-map ALL physical memory (tables located @8m)
* user tasks will have virtual addresses starting @1G, randomly mapped