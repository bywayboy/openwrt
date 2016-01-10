#
# Copyright (C) 2009 OpenWrt.org
#

BOARD:=arm64
SUBTARGET:=s5p6818
BOARDNAME:=S5P6818 ARMv8-a
ARCH_PACKAGES:=

CPU_TYPE:=cortex-a53
CPU_SUBTYPE:=neon-vfpv4

define Target/Description
	Build firmware image for Broadcom BCM2709 SoC devices.
endef

