#qemu-system-x86_64 -boot c -m 512 -drive file='bochs/c.img',index=0,media=disk,format=raw -net nic,vlan=0,model=ne2k_pci -netdev user,id=eusebius,vlan=0 -localtime -soundhw sb16 -smp cpus=2,cores=2 -vga std

#qemu-system-x86_64 -boot c -m 512 -drive file='bochs/c.img',index=0,media=disk,format=raw -net nic,model=ne2k_pci -netdev user,id=eusebius -localtime -soundhw sb16 -smp cpus=2,cores=2 -vga std

qemu-system-x86_64 \
 -boot c \
 -m 512 \
 -drive file='bochs/c.img',index=0,media=disk,format=raw \
 -vga std
### not now... takes too much time to reset between runs
### -netdev user,id=network0 -device rtl8139,netdev=network0,mac=52:54:00:12:34:56 \
### -device AC97 \
### -smp cpus=2,cores=2 \
# -device virtio-vga,virgl=on
# -soundhw hda \
# -soundhw sb16 \

# -net nic,model=ne2k_pci -netdev user,id=eusebius
# -soundhw hda
# -netdev user,id=network0 -device ne2k_pci,netdev=network0,mac=52:54:00:12:34:56
# -netdev user,id=network0 -device rtl8139,netdev=network0,mac=52:54:00:12:34:56 \


