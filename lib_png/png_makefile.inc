PNG_ORIG = $(PNG)png/

FINAL_CFLAGS += -I$(PNG_ORIG)

LIB_PNG = \
    png.o \
    pngerror.o \
    pngget.o \
    pngmem.o \
    pngpread.o \
    pngread.o \
    pngrio.o \
    pngrtran.o \
    pngrutil.o \
    pngset.o \
    pngtrans.o \
    pngwio.o \
    pngwrite.o \
    pngwtran.o \
    pngwutil.o

ifneq (,$(findstring arm,$(TARGET_ARCH)))
LIB_PNG += arm_init.o filter_neon.o filter_neon_intrinsics.o
endif

PNG_HEADERS = \
    $(PNG_ORIG)png.h \
    $(PNG_ORIG)pngconf.h \
    $(PNG_ORIG)pngdebug.h \
    $(PNG_ORIG)pnginfo.h \
    $(PNG_ORIG)pnglibconf.h \
    $(PNG_ORIG)pngpriv.h \
    $(PNG_ORIG)pngstruct.h

png.o: $(PNG_ORIG)png.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)png.c

pngerror.o: $(PNG_ORIG)pngerror.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngerror.c

pngget.o: $(PNG_ORIG)pngget.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngget.c

pngmem.o: $(PNG_ORIG)pngmem.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngmem.c

pngpread.o: $(PNG_ORIG)pngpread.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngpread.c

pngread.o: $(PNG_ORIG)pngread.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngread.c

pngrio.o: $(PNG_ORIG)pngrio.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngrio.c

pngrtran.o: $(PNG_ORIG)pngrtran.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngrtran.c

pngrutil.o: $(PNG_ORIG)pngrutil.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngrutil.c

pngset.o: $(PNG_ORIG)pngset.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngset.c

pngtrans.o: $(PNG_ORIG)pngtrans.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngtrans.c

pngwio.o: $(PNG_ORIG)pngwio.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngwio.c

pngwrite.o: $(PNG_ORIG)pngwrite.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngwrite.c

pngwtran.o: $(PNG_ORIG)pngwtran.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngwtran.c

pngwutil.o: $(PNG_ORIG)pngwutil.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)pngwutil.c

arm_init.o: $(PNG_ORIG)arm/arm_init.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)arm/arm_init.c

filter_neon.o: $(PNG_ORIG)arm/filter_neon.S $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)arm/filter_neon.S

filter_neon_intrinsics.o: $(PNG_ORIG)arm/filter_neon_intrinsics.c $(PNG_HEADERS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) $(PNG_DEFINES) -c $(PNG_ORIG)arm/filter_neon_intrinsics.c
