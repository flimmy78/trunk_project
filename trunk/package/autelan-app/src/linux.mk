#
#changed DIR_ROOT
#
export DIR_ROOT=$(shell pwd)/..

#
#don't change it
#
include $(DIR_ROOT)/mk/define.mk

#
#changed me, append sub-system
#
SUB_SYSTEM=partool appkey sinetd stimerc stimerd benv

#
#don't change it
#
include $(DIR_ROOT)/mk/template.mk