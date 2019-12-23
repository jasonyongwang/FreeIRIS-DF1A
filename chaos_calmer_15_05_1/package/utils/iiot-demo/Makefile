#
# Copyright (C) 2019 freeiris
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iiot-demo
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=FreeIRIS
  CATEGORY:=FreeIRIS
  TITLE:=$(PKG_NAME)
  DEPENDS:= +libpthread +libmosquitto-nossl +libmodbus
endef

define Package/$(PKG_NAME)/description
	Demo for IIOT
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/iiotdemo $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
