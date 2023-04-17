include $(TOPDIR)/rules.mk

# Name, version and release number
# The name and version of your package are used to define the variable to 
# point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=picod
PKG_VERSION:=1.0
PKG_RELEASE:=1
#PKG_BUILD_DIR:=$(BUILD_DIR)/build/release

# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=$(TOPDIR)/../picod
SOURCE_DIR:=$(realpath $(SOURCE_DIR))
TOOLCHAIN_DIR := $(shell find $(realpath $(STAGING_DIR)/../) -type d -name "toolchain-*")
CMAKE_C_COMPILER:=$(shell find $(realpath $(STAGING_DIR)/../) -type f -name $(TARGET_CC_NOCACHE)) 
CMAKE_CXX_COMPILER:=$(shell find $(realpath $(STAGING_DIR)/../) -type f -name $(TARGET_CXX_NOCACHE))

include $(INCLUDE_DIR)/package.mk

# Package definition; instructs on how and where our package will appear in 
# the overall configuration menu ('make menuconfig')
# DEPENDS:=+boost-libs +libconfig
define Package/picod
	SECTION:=base
	CATEGORY:=Utilities
	DEPENDS:=+libstdcpp +libcurl +libubox +libubus +libconfig +libpthread
	TITLE:=Raspberry Pi Pico monitoring daemon
endef

# Package description; a more verbose description on what our package does
define Package/picod/description
	Daemon for monitoring board temperatures and Watchdog via RPi Pico -application.
endef

# Package preparation instructions; create the build directory and copy the source code. 
# The last command is necessary to ensure our preparation instructions remain 
# compatible with the patching system.
define Build/Prepare
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	mkdir -p $(TOPDIR)/files/etc/init.d
	cp $(PKG_BUILD_DIR)/picod.conf $(TOPDIR)/files/etc/picod.conf
	cp $(PKG_BUILD_DIR)/picod_service $(TOPDIR)/files/etc/init.d/picod
	chmod +x $(TOPDIR)/files/etc/init.d/picod
	$(Build/Patch)
endef

# Package build instructions; invoke the target-specific compiler to first  
# compile the source file, and then to link the output into the final executable
################################################################
# make package/picod/clean && make package/picod/compile -j1 V=s
################################################################
define Build/Compile
	cd $(PKG_BUILD_DIR) && mkdir build && cd build &&\
	cmake -DCMAKE_C_COMPILER:FILEPATH=$(CMAKE_C_COMPILER) \
	-DCMAKE_CXX_COMPILER:FILEPATH=$(CMAKE_CXX_COMPILER) \
	-DTOOLCHAIN_DIR=$(TOOLCHAIN_DIR) ../ && make	
endef

# Package install instructions: 
# Create a directory inside the package to hold our executable, and then 
# copy the executable we built previously into the folder
define Package/picod/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/build/picod $(1)/usr/bin	
endef

# This command is always the last, it uses the definitions and variables that 
# we give above in order to get the job done
$(eval $(call BuildPackage,picod))