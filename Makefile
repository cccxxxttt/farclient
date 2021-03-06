#
# @bin farclient
# @author cxt <xiaotaohuoxiao@163.com>
# @start 2015-2-28
# @end   2015-3-18
#

include $(TOPDIR)/rules.mk

PKG_NAME:=farclient
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/farclient
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Use farclient to request and post the url
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)
endef

define Package/farclient/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/farclient $(1)/sbin/

	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) ./35-farclient $(1)/etc/hotplug.d/iface/35-farclient
	$(INSTALL_BIN) ./getUhttpdPort.sh $(1)/sbin/getUhttpdPort.sh

	$(INSTALL_DIR) $(1)/www/cgi-bin
	$(INSTALL_BIN) ./web_process $(1)/www/cgi-bin/web_process
endef

$(eval $(call BuildPackage,farclient))

