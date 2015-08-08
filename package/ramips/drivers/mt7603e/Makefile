# All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mt7603e
P4REV:=112670
PKG_VERSION:=p4rev-$(P4REV)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://localhost/
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_KCONFIG:= \
	RALINK_MT7620 RALINK_RT7621 MT_WIFI MT_WIFI_PATH\
	FIRST_MT_IF_EEPROM_PROM FIRST_MT_IF_EEPROM_EFUSE FIRST_MT_IF_EEPROM_FLASH RT_FIRST_CARD_EEPROM\
	MULTI_INF_SUPPORT MT_WSC_INCLUDED MT_WSC_V2_SUPPORT MT_DOT11N_DRAFT3 MT_DOT11_VHT_AC MT_DOT11W_PMF_SUPPORT\
	MT_TXBF_SUPPORT MT_WMM_ACM_SUPPORT MT_LLTD_SUPPORT MT_QOS_DLS_SUPPORT MT_WAPI_SUPPORT MT_CARRIER_DETECTION_SUPPORT\
	MT_IGMP_SNOOP_SUPPORT MT_BLOCK_NET_IF MT_RATE_ADAPTION MT_NEW_RATE_ADAPT_SUPPORT MT_AGS_SUPPORT\
	MT_IDS_SUPPORT MT_WIFI_WORK_QUEUE MT_WIFI_SKB_RECYCLE MT_RTMP_FLASH_SUPPORT MT_LED_CONTROL_SUPPORT\
	MT_SINGLE_SKU_V2 MT_HW_ANTENNA_DIVERSITY MT_ATE_SUPPORT MT_RT2860V2_AP_V24_DATA_STRUCTURE MT_RT2860V2_AP_32B_DESC\
	MT_MEMORY_OPTIMIZATION MT_HOTSPOT MT_HOTSPOT_R2 MT_UAPSD MT_PA_LNA RLT_BBP RLT_RF RTMP_BBP RTMP_RF\
	RTMP_PCI_SUPPORT RTMP_USB_SUPPORT RTMP_RBUS_SUPPORT MT_WIFI_MODE_AP RALINK_MT7603E MT_MAC MT_AP_SUPPORT\
	MT_WDS_SUPPORT MT_MBSS_SUPPORT MT_NEW_MBSSID_MODE MT_ENHANCE_NEW_MBSSID_MODE MT_APCLI_SUPPORT MT_MAC_REPEATER_SUPPORT\
	MT_LLTD_SUPPORT MT_NINTENDO_AP MT_COC_SUPPORT MULTI_CORE_SUPPORT
	

PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))

include $(INCLUDE_DIR)/package.mk

TAR_CMD=$(HOST_TAR) -C $(1)/ $(TAR_OPTIONS)

define KernelPackage/mt7603e
  CATEGORY:=MTK Properties
  TITLE:=MTK MT7603e wifi AP driver
  FILES:=$(PKG_BUILD_DIR)/build/mt7603e.ko
  AUTOLOAD:=$(call AutoLoad,91,mt7603e)
  SUBMENU:=Drivers
  MENU:=1
endef

define KernelPackage/mt7603e/config
	source "$(SOURCE)/config.in"
endef

define P4/Info
  P4URL:=//Embedded_WIFI/MP/MT7603_AlphaSite/rlt_wifi/
endef

define P4/Release
  make release
  mkdir -p build
  cp rlt_wifi_ap/Makefile.mt_wifi_ap build/Makefile
  cp rlt_wifi_ap/Kconfig.mt_wifi_ap build/Kconfig
  mv DPA src
  tar cjf mt7603e.tar.bz2 build src
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" V=1 \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)/build/" \
		$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_MT7603E_$c),CONFIG_$(c)=$(CONFIG_MT7603E_$(c)))) \
		modules
endef

define KernelPackage/mt7603e/install
	$(INSTALL_DIR) $(1)/lib/wifi/
	$(INSTALL_BIN) ./files/mt7603e.sh $(1)/lib/wifi/
	$(INSTALL_DIR) $(1)/etc/wireless/mt7603e/
	$(INSTALL_BIN) ./files/mt7603e.dat $(1)/etc/wireless/mt7603e/
	-$(INSTALL_BIN) ./files/mt7603e.eeprom.bin $(1)/etc/wireless/mt7603e/
	echo p$(P4REV) > $(1)/etc/wireless/mt7603e/version
endef

$(eval $(call KernelPackage,mt7603e))
