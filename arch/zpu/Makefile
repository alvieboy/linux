#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2004-2006 Atmel Corporation.

# Default target when executing plain make
.PHONY: all
all: vmlinux.elf

KBUILD_DEFCONFIG	:= zpuino_defconfig
CROSS_COMPILE := zpu-elf-

#-fno-inline -fno-builtin -fno-reorder-blocks  -fno-reorder-blocks-and-partition -fno-prefetch-loop-arrays -fno-gcse

KBUILD_CFLAGS	+= -pipe -O2 -mmult -mno-pushspadd
KBUILD_CFLAGS_REMOVE+= -fno-omit-frame-pointer
KBUILD_AFLAGS	+= -pipe -O2
KBUILD_CFLAGS_MODULE += 
LDFLAGS_vmlinux	+= -X --relax

KBUILD_CFLAGS	+= $(cpuflags-y)
KBUILD_AFLAGS	+= $(cpuflags-y)

CHECKFLAGS	+= -DZPU -D__BIG_ENDIAN

machine-$(CONFIG_PLATFORM_ZPU) := zcorev3
machdirs	:= $(patsubst %,arch/zpu/mach-%/, $(machine-y))

KBUILD_CPPFLAGS	+= $(patsubst %,-I$(srctree)/%include,$(machdirs))

head-y					+= arch/zpu/kernel/head.o
core-y					+= $(machdirs)
core-y					+= arch/zpu/kernel/
core-y					+= arch/zpu/mm/
libs-y					+= arch/zpu/lib/

BOOT_TARGETS := vmlinux.elf vmlinux.bin 

.PHONY: $(BOOT_TARGETS) install

boot := arch/$(ARCH)/boot/images

             KBUILD_IMAGE := $(boot)/vmlinux.bin

quiet_cmd_listing = LST     $@
      cmd_listing = zpu-elf-objdump $(OBJDUMPFLAGS) -lS $< > $@
quiet_cmd_disasm  = DIS     $@
      cmd_disasm  = zpu-elf-objdump $(OBJDUMPFLAGS) -d $< > $@

vmlinux.bin: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

install: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) BOOTIMAGE=$(KBUILD_IMAGE) $@

vmlinux.s: vmlinux
	$(call if_changed,disasm)

vmlinux.lst: vmlinux
	$(call if_changed,listing)

CLEAN_FILES += vmlinux.s vmlinux.lst

archclean:
	$(Q)$(MAKE) $(clean)=$(boot)

define archhelp
  @echo '* vmlinux.bin		- BIN image'
endef
