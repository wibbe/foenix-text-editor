RAMSTART = 0x00020000;
RAMSIZE  = 0x00030000;
STACKLEN = 0x400;
BINFILESTART = 0x00000000;
PAGESIZE = 0x10000;

MEMORY
{
    ram : org = RAMSTART, len = RAMSIZE - STACKLEN
    binpages: org = BINFILESTART, len = PAGESIZE
}

SECTIONS
{
  text : {*(CODE)} >ram AT>binpages
  .dtors : { *(.dtors) } > ram AT>binpages
  .ctors : { *(.ctors) } > ram AT>binpages
  rodata : {*(RODATA)} >ram AT>binpages
  data: {*(DATA)} >ram
  bss (NOLOAD): {*(BSS)} >ram

  ___heap = ADDR(bss) + SIZEOF(bss);
  ___heapend = RAMSTART + RAMSIZE - STACKLEN;


  ___BSSSTART = ADDR(bss);
  ___BSSSIZE  = SIZEOF(bss);

  ___STACK = RAMSTART + RAMSIZE;
}

