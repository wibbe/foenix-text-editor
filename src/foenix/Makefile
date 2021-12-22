a_src = $(wildcard *.s)
a_obj = $(subst .s,.o,$(a_src))
c_str = $(wildcard *.c)
h_src := $(wildcard *.h)
c_obj = $(subst .c,.o,$(c_str))

.PHONY: all
all: $(c_obj) $(a_obj) $(h_src)

%.o: $.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFINES)

.PHONY: clean

clean:
	$(RM) $(a_obj) $(c_obj) *.asm
