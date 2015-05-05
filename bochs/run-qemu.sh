qemu-system-x86_64 -boot c -m 512 -hda 'c.img' -net nic,vlan=0 -net user,vlan=0 -localtime -soundhw sb16 -smp 4 -vga std
