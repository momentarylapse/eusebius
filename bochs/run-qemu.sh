qemu-system-x86_64 -boot c -m 512 -drive file='c.img',index=0,media=disk,format=raw -net nic,vlan=0 -net user,vlan=0 -localtime -soundhw sb16 -smp cpus=2,cores=2 -vga std
