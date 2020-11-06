CXSRC = $(CHIBIX)/src/cx.c \
        $(CHIBIX)/src/cxdispatch.c \
        $(CHIBIX)/src/cxmonitor.c

CXINC = $(CHIBIX)/include

# Shared variables
ALLCSRC += $(CXSRC)
ALLINC  += $(CXINC)
