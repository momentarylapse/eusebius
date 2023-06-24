# Design

## Memory

### Address space

	----------------------------------- kernel space (id-mapped)
	0x0000 7c00:0x0000 7e00    loader code (512b)
	0x0000 7e00:...            init code
	0x0001 0000:0x0005 0000    kernel code (max 256k)
	0x0005 0000:...            kalib code
	0x0010 0000:...            kernel variables
	  0x0028 0000:0x0080 0000    kernel heap (max 5.5m)
	0x00a3 f000:...            kalib variables
	----------------------------------- user space (randomly mapped)
	0x0080 0000    user task code (max 512k)
	0x0088 0000    user task variables

### Paging