#
# Copyright (C) 2014 MediaTek
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=exfat-nofuse
PKG_VERSION:=2014-11-25
PKG_RELEASE:=PKG_SOURCE_VERSION

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/dorimanx/exfat-nofuse.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_VERSION:=a862d506bd776c69b85741146e46f420dba78d00

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_LICENSE:=GPLv2
PKG_KCONFIG:=EXFAT_FS EXFAT_DISCARD EXFAT_DELAYED_SYNC EXFAT_KERNEL_DEBUG \
	EXFAT_DEBUG_MSG EXFAT_DEFAULT_CODEPAGE EXFAT_DEFAULT_IOCHARSET

PKG_MAINTAINER:=Hua Shao <hua.shao@mediatek.com>

include $(INCLUDE_DIR)/package.mk


define KernelPackage/exfat-nofuse
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:=MS exFAT support
  FILES:=$(PKG_BUILD_DIR)/exfat.ko
  AUTOLOAD:=$(call AutoProbe,exfat)
  DEPENDS:=+kmod-nls-base
  MENU:=1
endef

define KernelPackage/exfat-nofuse/config
	source "$(SOURCE)/config.in"
endef

define Build/Compile
	$(MAKE) -C "$(PKG_BUILD_DIR)" \
		KDIR="$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)=$(CONFIG_$(c)))) \
		all
endef

$(eval $(call KernelPackage,exfat-nofuse))
