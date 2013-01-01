#
# Copyright (C) 2012 Alvaro Lopes
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#

OBJCOPYFLAGS_vmlinux.bin := -O binary -R .note.gnu.build-id
targets += vmlinux.bin

$(obj)/vmlinux.bin: vmlinux FORCE
	$(call if_changed,objcopy)

# Generated files to be removed upon make clean
clean-files	:= vmlinux.bin
