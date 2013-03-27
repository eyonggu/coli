#This is an example of Makefile

PROGRAM = rwscolish
ARCH    = i686
OS      = linux

#header directory
INCDIRS += 
INCDIRS += ../../
ifeq ($(ARCH), powerpc)
INCDIRS += $(RCSIFDIR)/rcs-libedit/include
else
INCDIRS += /home/eyonggu/include
endif

#source directory
SRCDIRS += .
SRCDIRS += ../../

ifeq ($(ARCH), powerpc)
LIBDIRS += $(RCSIFDIR)/rcs-libedit/lib/$(ARCH)
else
LIBDIRS += /home/eyonggu/lib
endif

LDLIBS += edit

ifeq ($(ARCH), powerpc)
include /home/eyonggu/github/commk/dusg2.mk
endif
include /home/eyonggu/github/commk/Common.mk
