= x64 architecture infos

== Special registers

    cr0
     0   pe      protection enabled
     1   mp      monitor coprocessor
     2   em      emulation
     3   ts      task switched
     4   et      extension type
     5   ne      numeric error
     16  wp      write protect
     18  am      alignment mask
     29  nw      not write-through
     30  cd      cache disabled
     31  pg      paging

    cr2
     page fault virtual address

    cr3
     page level 4/5 root

    cr4
     0   vme     virtual 8086 mode
     1   pvi     protected mode virtual interrupts
     2   tsd     time stamp disabled
     3   de      debug extension
     4   pse     page size extenion
     5   pae     physical address extension
     6   mce     machine check enabled
     7   pge     page global enabled
     8   pce     performance counter enabled
     9   osfxsr  os fxsave support (sse)
     10  osxmmexcpt os unmasked exceptions support
     11  umip    user mode instruction prevention
     12  la57    5 level paging enabled

    rflags
     0   cf      carry flag
     2   pf      parity flag
     4   af      auxiliary flag
     6   zf      zero flag
     7   sf      sign flag
     8   tf      trap flag ("single stepping")
     9   if      interrupt flag
     10  df      direction flag
     11  of      overflow flag
     12:13 iopl  io privilege level
     14  nt      nested task
     16  rf      resume flag
     17  vm      virtual 8086 mode
     18  ac      alignment check
     19  vif     virtual interrupt flag
     20  vip     virtual interrupt pending
     21  id      id flag
     
    efer 0xc0000080
     0   sce     syscall extension
     8   lme     long mode enabled
     10  lma     long mode active
     11  nxe     no-execute enabled
     12  svme    secure virtual machine enabled
     13  lmsle   long mode segment limit enabled
     14  ffxsr   fast fxsave
     15  tce     translation cache extension
     17  mcommint enable mcommit instruction
     18  intwp   interruptible wbinvd enabled
     20  uaie    upper address ignore enabled
     21  aibrse  automatic ibrs enabled

== Syscall

    syscall:
     (cs = STAR...)
     (ss = STAR...)
     (rflags = ..MSTAR)
     (cpl = 0)
     rcx = rip
     r11 = rflags
     rip = LSTAR

    sysret:
     (cs = STAR...)
     (ss = STAR...)
     (cpl = 3)
     rflags = r11
     rip = rcx


== Interrupt

