include $(TOPDIR)/rules.mk

PKG_NAME:=uvhttpd
PKG_VERSION:=0.0.1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=uwrt
  CATEGORY:=uWRT
  TITLE:=uvhttpd based on libuv.
  URL:=http://www.extgui.com
  DEPENDS:= +libuv +liblua +libuci +libubus
endef

define Package/$(PKG_NAME)/description
	uWRT Web Server Package For MT7620. <bywayboy@gmail.com>
endef

#编译准备工作,复制源码到目标目录
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./files $(PKG_BUILD_DIR)/
	$(Build/Patch)
endef

#[安装(编译)时执行的脚本 记得加上#!/bin/sh 没有就空着]
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/uvhttpd $(1)/usr/sbin/uvhttpd

	$(INSTALL_DIR) $(1)/etc/config
	$(CP) $(PKG_BUILD_DIR)/files/etc/config/uvhttpd $(1)/etc/config/uvhttpd

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/uvhttpd $(1)/etc/init.d/uvhttpd

	$(INSTALL_DIR) $(1)/etc/rc.d
	ln -s ../init.d/uvhttpd $(1)/etc/rc.d/S90uvhttpd

endef

$(eval $(call BuildPackage,uvhttpd))