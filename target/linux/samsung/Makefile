#
# Copyright (C) 2015-20xx extgui.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=samsung
BOARDNAME:=Samsung S5P4418/S5P6818
FEATURES:=ext4 audio display fpu gpio pcie rtc usb usbgadget targz ubifs
SUBTARGETS=s5p4418 s5p6818
MAINTAINER:=bywayboy <bywayboy@qq.com>

KERNEL_PATCHVER:=4.4

KERNELNAME:=Image dtbs zImage

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += \
	kmod-leds-gpio kmod-gpio-button-hotplug swconfig

$(eval $(call BuildTarget))
