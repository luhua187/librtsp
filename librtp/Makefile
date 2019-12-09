#--------------------------------Output------------------------------
# OUTTYPE: 0-exe, 1-dll, 2-static
#--------------------------------------------------------------------

OUTTYPE = 2
OUTFILE = librtp.a

CFLAGS += -I./source/ -I./include -I./payload  -shared -fPIC
#-------------------------------Include------------------------------
#
# INCLUDES = $(addprefix -I,$(INCLUDES)) # add -I prefix
#--------------------------------------------------------------------


#-------------------------------Source-------------------------------
#
#--------------------------------------------------------------------
SOURCE_PATHS = source payload
SOURCE_FILES = $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.cpp))
SOURCE_FILES += $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.c))
SOURCE_OBJS0 = $(subst .cpp,.o,$(SOURCE_FILES))
SOURCE_OBJS = $(subst .c,.o,$(SOURCE_OBJS0))
#-----------------------------Library--------------------------------
#
# LIBPATHS = $(addprefix -L,$(LIBPATHS)) # add -L prefix
#--------------------------------------------------------------------
LIBPATHS =
ifdef RELEASE
# relase library path
LIBPATHS +=
else
LIBPATHS +=
endif

LIBS = 

STATIC_LIBS =

#-----------------------------DEFINES--------------------------------
#
# DEFINES := $(addprefix -D,$(DEFINES)) # add -L prefix
#--------------------------------------------------------------------
DEFINES =

#include ../gcc.mk

CC=gcc
CXX=g++
AR=ar 

all: $(OUTFILE)
	

clean: 
	rm -fr $(OUTFILE) $(SOURCE_OBJS)


$(OUTFILE):$(SOURCE_OBJS)
	$(AR) r $@ $^

.c.o:
	@$(CC) $(CFLAGS) -c -o $@ $^

.cpp.o:
	@$(CXX) $(CFLAGS) -c -o $@ $^


