#
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mipsel
BOARD:=ramips
BOARDNAME:=MTK/Ralink APSoC
SUBTARGETS:=mt7620 mt7621 mt7628
FEATURES:=squashfs gpio

LINUX_P4PATH:=//WIFI_SOC/MP/SDK_4_3_0_0/RT288x_SDK/source
LINUX_P4REV:=112871
LINUX_VERSION:=3.10.14-p$(LINUX_P4REV)

include $(INCLUDE_DIR)/target.mk
DEFAULT_PACKAGES += 

define Target/Description
	Build firmware images for MTK/Ralink APSoC based boards.
endef

$(eval $(call BuildTarget))
