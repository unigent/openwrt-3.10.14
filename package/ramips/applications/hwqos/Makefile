#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=hwqos
PKG_RELEASE:=4210

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=RALINK_MT7621 
PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))


include $(INCLUDE_DIR)/package.mk

define Package/hwqos
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=Hardware QoS command
  DEPENDS:=@PACKAGE_kmod-hw_nat @TARGET_ramips_mt7621 +libnvram
  SUBMENU:=Applications
endef

define Package/hwqos/description
  A program to work with driver
endef

define Build/Compile
	@echo "not source code provided"
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) ./src/*.h $(1)/usr/include
endef

define Package/hwqos/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./src/qos_run $(1)/usr/sbin
	$(CP) ./src/qos_portbased_run $(1)/usr/sbin
endef


$(eval $(call BuildPackage,hwqos))

