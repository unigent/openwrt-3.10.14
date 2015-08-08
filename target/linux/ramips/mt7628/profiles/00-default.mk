#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/MT7628
	NAME:=MT7628 Default
	PACKAGES:= -swconfig -rt2x00 \
		ated reg gpio btnd switch ethstt uci2dat mii_mgr watchdog 8021xd eth_mac \
		wireless-tools xl2tpd block-mount fstools kmod-scsi-generic \
		kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-usb-storage \
		kmod-fs-vfat kmod-fs-ntfs kmod-nls-base kmod-nls-utf8 kmod-nls-cp936 \
        kmod-nls-cp437 kmod-nls-cp850 kmod-nls-iso8859-1 kmod-nls-iso8859-15 kmod-nls-cp950
endef

define Profile/MT7628/Description
	Basic MT7628 SoC support
endef
$(eval $(call Profile,MT7628))



