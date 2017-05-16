#TARGET_OS = linux_sdl
#TARGET_OS = linux_sdl_raspberry_pi
#TARGET_OS = linux_x11_opengl
TARGET_OS = linux_x11_opengles
#TARGET_OS = linux_x11_opengles_raspberry_pi
#TARGET_OS = linux_android
#TARGET_OS = freebsd
#TARGET_OS = windows
#TARGET_OS = windows_opengl
#TARGET_OS = windowsce
#TARGET_ARCH = x86
#TARGET_ARCH = x86_64
TARGET_ARCH = arm

ROOT = ../../
SUNDOG = $(ROOT)lib_sundog/
JPEG = $(ROOT)lib_jpeg/
PNG = $(ROOT)lib_png/
GIF = $(ROOT)lib_gif/
ZLIB = $(ROOT)lib_zlib/
DSP = $(ROOT)lib_dsp/
PIXILANG = $(ROOT)lib_pixilang/

APPNAME = pixilang

include $(SUNDOG)sundog_makefile.inc
include $(JPEG)jpeg_makefile.inc
include $(PNG)png_makefile.inc
include $(GIF)gif_makefile.inc
include $(ZLIB)zlib_makefile.inc
include $(DSP)dsp_makefile.inc
include $(PIXILANG)pixilang_makefile.inc

## ------------------------------------------ ##
## Main target options:                       ##
## ------------------------------------------ ##

#LDFLAGS = -s
CFLAGS = -O3
#-ftree-vectorizer-verbose=2
#CFLAGS = -g

ifneq (,$(findstring linux,$(TARGET_OS)))
ifeq ($(TARGET_OS),$(filter $(TARGET_OS),linux_sdl linux_sdl_raspberry_pi))
    LIBS = -L/usr/X11R6/lib -lX11
    SUNDOG_DEFINES = \
	-D$(COLOR_TYPE) \
	-DDIRECTDRAW \
	-DNOVCAP \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../release/pixilang/pixilang/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_x11_opengl)
    LIBS = -L/usr/X11R6/lib -lX11 -lGL -lXext
    SUNDOG_DEFINES = \
	-DX11 \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE) \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../release/pixilang/pixilang/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_x11_opengles)
    LIBS = -L/usr/X11R6/lib -lX11 -lEGL -lGLESv2
    SUNDOG_DEFINES = \
	-DX11 \
	-DOPENGL \
	-DOPENGLES \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE) \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../release/pixilang/pixilang/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_x11_opengles_raspberry_pi)
    LIBS = -lbcm_host -lvcos -lvchiq_arm -L/usr/X11R6/lib -lX11 -lEGL -lGLESv2
    SUNDOG_DEFINES = \
	-DX11 \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE) \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../release/pixilang/pixilang/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_android)
    LIBS = -lEGL -lGLESv2
    SUNDOG_DEFINES = \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-D$(COLOR_TYPE) \
	-DFREE_VERSION \
	-DPKGNAME="\"nightradio/$(APPNAME)\""
dump:
	$(OBJDUMP) -D $(APPNAME).so >$(APPNAME).asmdump
ifeq ($(FLOAT_ABI),softfp)
install:
	mv -f $(APPNAME).so ./android_project/app/src/main/jniLibs/armeabi-v7a/libsundog.so
else
ifeq ($(TARGET_ARCH),arm)
install:
	mv -f $(APPNAME).so ./android_project/app/src/main/jniLibs/armeabi/libsundog.so
else
install:
	mv -f $(APPNAME).so ./android_project/app/src/main/jniLibs/x86/libsundog.so
endif
endif
endif
endif
ifeq ($(TARGET_OS),windows_opengl)
    LIBS = -lgdi32 -lopengl32
    SUNDOG_DEFINES = \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE)
install:
	mv -f $(APPNAME).exe ../release/pixilang/pixilang/windows/$(APPNAME)$(APPNAME_SUFFIX).exe
endif
ifeq ($(TARGET_OS),windows)
    LIBS = -lgdi32
    SUNDOG_DEFINES = \
	-DGDI \
	-DNOVCAP \
	-D$(COLOR_TYPE)
install:
	mv -f $(APPNAME).exe ../release/pixilang/pixilang/windows/$(APPNAME)$(APPNAME_SUFFIX).exe
endif
ifeq ($(TARGET_OS),windowsce)
    LIBS =
    SUNDOG_DEFINES = \
	-D$(COLOR_TYPE) \
	-DDIRECTDRAW \
	-DGDI \
	-DNOVCAP \
	-DSHOW_PIXILANG_MENU
install:
	mv -f $(APPNAME).exe ../release/pixilang/pixilang/wince
endif

## ------------------------------------------ ##
## Object files and includes:                 ##
## ------------------------------------------ ##

# User application:
LIB_PIXILANG_MAIN = 
ADD_LIBS = \
    $(LIB_PIXILANG_MAIN) \
    $(LIB_JPEG) \
    $(LIB_PNG) \
    $(LIB_GIF) \
    $(LIB_ZLIB) \
    $(LIB_DSP) \
    $(LIB_PIXILANG)
OBJS += user_code.o user_code_info.o $(ADD_LIBS)

## ------------------------------------------ ##
## User application rules:                    ##
## ------------------------------------------ ##

user_code.o: ../main/user_code.cpp $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c ../main/user_code.cpp

user_code_info.o: ../main/user_code_info.cpp $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c ../main/user_code_info.cpp
