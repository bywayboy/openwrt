#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/Default
	NAME:=NanoPi3
	PACKAGES:=\
		kmod-usb-core kmod-usb2 kmod-usb-ohci \
		kmod-ledtrig-usbdev
endef

define Profile/Default/Description
	FriendlyARM NanoPI3 Board.
endef
$(eval $(call Profile,Default))
 
