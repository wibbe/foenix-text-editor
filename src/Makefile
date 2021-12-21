
#
# User may over-ride the UNIT and MEMORY parameters to specify target machine
# and where the MCP will run (ram or flash)
#
UNIT := a2560u

# CPU_WDC65816                0x16  /* CPU code for the Western Design Center 65816 */
# CPU_M68000                  0x20  /* CPU code for the Motorola 68000 */
# CPU_M68010                  0x21  /* CPU code for the Motorola 68010 */
# CPU_M68020                  0x22  /* CPU code for the Motorola 68020 */
# CPU_M68030                  0x23  /* CPU code for the Motorola 68030 */
# CPU_M68040                  0x24  /* CPU code for the Motorola 68040 */
# CPU_I486DX                  0x34  /* CPU code for the Intel 486DX */

# MODEL_FOENIX_FMX            0
# MODEL_FOENIX_C256U          1
# MODEL_FOENIX_GENX           4
# MODEL_FOENIX_C256U_PLUS     5
# MODEL_FOENIX_A2560U_PLUS    6
# MODEL_FOENIX_A2560X         8
# MODEL_FOENIX_A2560U         9
# MODEL_FOENIX_A2560K         13

# Determine target CPU and MODEL based on the UNIT

ifeq ($(UNIT),a2560k)
	export CPU_NUMBER = 36		# M68040
	export VASM_CPU = -m68040	# VASM CPU flag
	export VBCC_CPU = 68040		# VBCC CPU flag
	export MODEL_NUMBER = 13	# A2560K
	export cpu = m68040
else ifeq ($(UNIT),a2560u)
	export CPU_NUMBER = 32		# M68000
	export VASM_CPU = -m68000	# VASM CPU flag
	export VBCC_CPU = 68000		# VBCC CPU flag
	export MODEL_NUMBER = 9		# A2560U
	export CFG_FILE = $(VBCC)/config/a2560u_ram
	export cpu = m68k
endif

export AS = vasmm68k_mot
export ASFLAGS = $(VASM_CPU) -quiet -Fvobj -nowarn=62
export CC = vc
export DEFINES = -DCPU=$(CPU_NUMBER) -DMODEL=$(MODEL_NUMBER)

ifeq ($(OS),Windows_NT)
	export CFLAGS = -cpu=$(VBCC_CPU) +$(CFG_FILE) -I. -I$(CURDIR) -I$(CURDIR)/syscall
	export RM = cmd /C del /Q /F
else
	export CFLAGS = -cpu=$(VBCC_CPU) +$(CFG_FILE)_linux -I. -I$(CURDIR) -I$(CURDIR)/syscall
	export RM = rm -f
endif

syscall_c_src := $(wildcard syscall/*.c)
syscall_c_obj := $(subst .c,.o,$(syscall_c_src))
c_src :=  $(wildcard *.c)
c_obj := $(subst .c,.o,$(c_src))

.PHONY: all syscall

all: fte.bin syscall

syscall:
	$(MAKE) --directory=syscall

fte.bin: $(c_obj) syscall
	$(CC) $(CFLAGS) $(DEFINES) -o fte.bin $(c_obj) $(syscall_c_obj)

%.o: %.c $(DEPS)
	$(CC) -S -c -o $@ $< $(CFLAGS) $(DEFINES)

.PHONEY: clean

clean:
	$(RM) *.bin *.o *.asm
	$(MAKE) --directory=syscall clean
