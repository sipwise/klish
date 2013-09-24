#############################################################
#
# klish
#
#############################################################

ifeq ($(BR2_PACKAGE_KLISH_SVN),y)
KLISH_VERSION:=HEAD
KLISH_SITE:=http://klish.googlecode.com/svn/trunk
KLISH_SITE_METHOD:=svn
else
KLISH_VERSION = 1.6.0
KLISH_SOURCE = klish-$(KLISH_VERSION).tar.bz2
KLISH_SITE = http://klish.googlecode.com/files
endif

KLISH_INSTALL_STAGING = YES
KLISH_INSTALL_TARGET = YES
KLISH_CONF_OPT = --disable-gpl --without-tcl

$(eval $(call AUTOTARGETS,package,klish))
