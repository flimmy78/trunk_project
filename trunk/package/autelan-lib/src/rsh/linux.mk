#
#don't change it
#
DIR_SELF=$(shell pwd)

#
#changed DIR_ROOT
#
export DIR_ROOT=$(DIR_SELF)/../..

#
#don't change it
#
include $(DIR_ROOT)/mk/define.mk

#
#changed me, append obj
#
OBJS=$(DIR_SELF)/rsh.o
TARGET_NAME=rsh
SONAME=lib$(TARGET_NAME).so
SONAME_MASTER=$(SONAME).1
TARGET=$(SONAME_MASTER).0
TARGET_TYPE=lib
LIB_DEPEND=c appkey
CFLAGS+=-DPART_XOR=0

#
#don't change it
#
include $(DIR_ROOT)/mk/common.mk
