asources = $(wildcard *.s)
aobjects = $(subst .s,.o,$(asources))
csources = $(wildcard *.c)
cobjects = $(subst .c,.o,$(csources))

.PHONY: all
all: $(cobjects) $(aobjects)

%.o: $.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFINES)

.PHONY: clean

clean:
	$(RM) $(aobjects) $(cobjects) *.asm
