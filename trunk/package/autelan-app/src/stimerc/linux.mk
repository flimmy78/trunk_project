#
#don't change it
#
DIR_SELF=$(shell pwd)

#
#changed DIR_ROOT
#
DIR_ROOT=$(DIR_SELF)/../..

#
#don't change it
#
include $(DIR_ROOT)/mk/define.mk

#
#changed me, append obj
#
OBJS=$(DIR_SELF)/stimerc.o
TARGET_NAME=stimerc
TARGET=$(TARGET_NAME)
TARGET_TYPE=exe
LIB_DEPEND=c appkey timer
CFLAGS+=-DSTIMERC -DSTIMER_TICKS=1000 -DSTIMER_TIMEOUT_TICKS=5000

#
#don't change it
#
include $(DIR_ROOT)/mk/common.mk
