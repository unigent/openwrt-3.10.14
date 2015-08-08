#
# Copyright (C) 2010-2014 hua.shao@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=nvram
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=NVRAM_MTD_NAME

include $(INCLUDE_DIR)/package.mk

define Package/nvram
  CATEGORY:=MTK Properties
  SUBMENU:=Applications
  TITLE:=MTK nvram tool
  DEPENDS:=@TARGET_ramips +libc +libnvram
  MENU:=1
endef

define Package/libnvram
  CATEGORY:=MTK Properties
  SUBMENU:=Libraries
  TITLE:=MTK nvram library
  DEPENDS:=@TARGET_ramips +libc
endef

define Package/nvram/description
  MTK nvram tool
endef

define Package/libnvram/description
  MTK nvram library
endef

define Package/nvram/config
	source "$(SOURCE)/config.in"
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call Build/Prepare/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP)   ./src/nvram.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP)	$(PKG_BUILD_DIR)/lib*.so $(1)/usr/lib/
endef


MAKE_FLAGS += \
	CONFIG_NVRAM_MTD_NAME=$(CONFIG_NVRAM_MTD_NAME)


define Package/nvram/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nvram $(1)/usr/bin
endef

define Package/libnvram/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/*.so $(1)/usr/lib
endef


$(eval $(call BuildPackage,libnvram))
$(eval $(call BuildPackage,nvram))

