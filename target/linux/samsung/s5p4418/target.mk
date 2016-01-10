#
# Copyright (C) 2009 OpenWrt.org
#

SUBTARGET:=s5p4418
BOARDNAME:=S5P4418

#ARCH_PACKAGES:=

CPU_TYPE:=cortex-a9
CPU_SUBTYPE:=neon-vfpv3


#DEFAULT_PACKAGES += kmod-rt2800-pci kmod-rt2800-soc kmod-mt76

define Target/Description
	Build firmware images for Samsung S5P6818 based boards.
endef

