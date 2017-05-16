##
## Defined in the app MakeFile:
##   TARGET_OS
##   TARGET_ARCH - architecture name: arm, arm64, x86, x86_64, mips, mips64; NOT ABI NAME! ABI can be selected by some additional options;
##   ROOT
##   SUNDOG - base library (engine)
##   SUNDOG_DEFINES
##   APPNAME
##   LIBS - additional libraries; for example: -lasound -lGL
##

##
## Make options:
## (example: MAKE_WITHOUT_SIMD=true)
##   MAKE_DYNAMIC_LIB
##   MAKE_STATIC_LIB
##   MAKE_WITH_CONSOLE
##   MAKE_WITH_INTEL_COMPILER
##   MAKE_WITH_MULTITOUCH
##   MAKE_WITHOUT_MAIN
##   MAKE_WITHOUT_WM
##   MAKE_WITHOUT_SIMD
##   MAKE_FOR_SLOW_CPU
##

##
## Target configuration
##

# LDFLAGS - must be set by user
# CFLAGS - must be set by user
CFLAGS2 = 
LDFLAGS2 = 
DEFAULT_LIBS = 

##
## Linux
##
ifneq (,$(findstring linux,$(TARGET_OS)))
    CC = gcc
    CXX = $(CC)
    LD = $(CC)
    CFLAGS2 += -DLINUX -Wall -Wno-unused-variable -Wno-multichar -pipe
    DEFAULT_LIBS = -lpthread -lrt -lstdc++ -lasound -lSDL -lm -ldl
##
## Linux: Raspberry PI
##
ifneq (,$(findstring raspberry_pi,$(TARGET_OS)))
    SDK = /opt/vc
    CFLAGS2 += -DRASPBERRY_PI -I$(SDK)/include -I$(SDK)/include/interface/vcos/pthreads -I$(SDK)/include/interface/vmcs_host/linux
    LDFLAGS2 += -L$(SDK)/lib
endif
##
## Linux: Maemo
##
ifneq (,$(findstring maemo,$(TARGET_OS)))
    CFLAGS2 += -DMAEMO
endif
##
## Linux: Android
##
ifneq (,$(findstring android,$(TARGET_OS)))
    NDK = /home/alex/soft/android-ndk-r10e
    SYSROOT = $(NDK)/platforms/android-9/arch-$(TARGET_ARCH)
    CFLAGS2 += \
	-DANDROID \
	-DGL_GLEXT_PROTOTYPES=1 \
	-I$(SYSROOT)/usr/include \
	-I$(NDK)/sources/android/native_app_glue
    DEFAULT_LIBS = -llog -landroid -lOpenSLES -lm -ldl
ifeq ($(TARGET_ARCH),x86) 
ifeq ($(MAKE_WITH_INTEL_COMPILER),true)
    CC = icc -platform=android --sysroot=$(SYSROOT)
    CXX = $(CC)
    LD = $(CC)
    CFLAGS2 += \
	-fomit-frame-pointer \
	-fstrict-aliasing \
	-finline-limit=300 \
	-ffunction-sections \
	-restrict \
	-fno-exceptions \
	-fno-rtti
else
    CC = $(NDK)/toolchains/x86-4.8/prebuilt/linux-x86_64/bin/i686-linux-android-gcc --sysroot=$(SYSROOT)
    CXX = $(CC)
    LD = $(CC)
    OBJDUMP = $(NDK)/toolchains/x86-4.8/prebuilt/linux-x86_64/bin/i686-linux-android-objdump
endif
endif
ifeq ($(TARGET_ARCH),arm)
    CC = $(NDK)/toolchains/arm-linux-androideabi-4.8/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc --sysroot=$(SYSROOT)
    CXX = $(CC)
    LD = $(CC)
    OBJDUMP = $(NDK)/toolchains/arm-linux-androideabi-4.8/prebuilt/linux-x86_64/bin/arm-linux-androideabi-objdump
endif
ifeq ($(TARGET_ARCH),arm64)
    CC = $(NDK)/toolchains/aarch64-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/aarch64-linux-androideabi-gcc --sysroot=$(SYSROOT)
    CXX = $(CC)
    LD = $(CC)
    OBJDUMP = $(NDK)/toolchains/aarch64-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/aarch64-linux-androideabi-objdump
endif
    MAKE_DYNAMIC_LIB = true
endif #android
endif #linux
##
## OSX
##
ifeq ($(TARGET_OS),osx)
    CC = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
    CXX = $(CC)
    LD = $(CC)
    CFLAGS2 = \
	-DOSX \
	-Wno-multichar \
	-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk \
	-mmacosx-version-min=10.6 \
	-pipe
    DEFAULT_LIBS = -lpthread -lstdc++ -framework Cocoa -framework Carbon -framework AudioUnit -framework CoreAudio
endif
##
## iOS
##
ifeq ($(TARGET_OS),iphone)
    PLATFORM_BIN = /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/
    TOOLCHAIN_BIN = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/
ifneq (,$(findstring x86,$(TARGET_ARCH)))
    SDK = /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator10.0.sdk
else
    SDK = /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS10.0.sdk
endif
    CC = $(TOOLCHAIN_BIN)clang
    CXX = $(CC)
    CMM = $(CC) -x objective-c++
    LD = $(CC)
    LIBTOOL = $(TOOLCHAIN_BIN)libtool -syslibroot $(SDK)
    CFLAGS2 = \
	-DIOS \
	-DIPHONE \
	-Wno-multichar \
	-isysroot $(SDK) \
	-miphoneos-version-min=6.0 \
	-fembed-bitcode \
	-pipe
    DEFAULT_LIBS = -framework Foundation -framework AudioToolbox -framework AVFoundation
endif
##
## Windows
##
ifneq (,$(findstring windows,$(TARGET_OS)))
ifneq ($(TARGET_OS),windowsce)
    CC = i686-w64-mingw32-gcc
    CXX = $(CC)
    LD = $(CC)
    WINDRES = i686-w64-mingw32-windres
    FILE_EXT = .exe
    CFLAGS2 = \
	-DWIN \
	-malign-double \
	-fno-exceptions \
	-Wall -Wno-unused-variable -Wno-multichar \
	-pipe \
	-Iresources/
    DEFAULT_LIBS = -ldsound -ldxguid -lwinmm -lole32 -lstdc++
endif
endif
##
## WindowsCE
##
ifeq ($(TARGET_OS),windowsce)
    CC = arm-mingw32ce-gcc
    CXX = $(CC)
    LD = $(CC)
    WINDRES = arm-mingw32ce-windres
    FILE_EXT = .exe
    CFLAGS2 = \
	-DWINCE \
	-fno-exceptions \
	-Wno-multichar \
	-pipe \
	-Iresources/ \
	-I$(SUNDOG)various/wince/
    DEFAULT_LIBS = -laygshell -lceshell -lstdc++ -static
endif

##
## x86
##
ifeq ($(TARGET_ARCH),x86)
    CFLAGS2 += -DARCH_X86 -DARCH_NAME="\"x86\"" 
ifneq (,$(findstring android,$(TARGET_OS)))
    CFLAGS2 += -DCPUMARK=0 
else
ifeq ($(MAKE_FOR_SLOW_CPU),true)
    CFLAGS2 += -DCPUMARK=0 
else
    CFLAGS2 += -DCPUMARK=10 
endif
endif
ifeq ($(MAKE_WITH_INTEL_COMPILER),true)
    CFLAGS2 += -xATOM_SSSE3 -ipo
    LDFLAGS2 += -xATOM_SSSE3 -ipo
else
    CFLAGS2 += -m32 -ffast-math
    LDFLAGS2 += -m32
    LIBTOOL_FLAGS = -static -arch_only i686
ifneq (,$(findstring android,$(TARGET_OS)))
    CFLAGS2 += -mssse3
endif
ifeq ($(MAKE_WITHOUT_SIMD),true)
    CFLAGS2 += -march=i686
else
ifneq (,$(findstring windows,$(TARGET_OS)))
#temp hack: Filter Pro + ffast-math + march=pentium3 (min) crash the Windows 10 machines; (i686-w64-mingw32-gcc 4.9.1)
#also it seems there is no big difference between the pure i686 and i686+sse+vectorize :) but we should check it more...
    CFLAGS2 += -march=i686
else
    CFLAGS2 += -march=i686 -mfpmath=sse -msse3 -ftree-vectorize
endif #not windows
endif #with SIMD
endif #not intel c++
endif #x86
##
## x86_64
##
ifeq ($(TARGET_ARCH),x86_64) 
    CFLAGS2 += -DARCH_X86_64 -DARCH_NAME="\"x86_64\"" -m64 -ffast-math -mfpmath=sse -msse3 -ftree-vectorize
ifeq ($(MAKE_FOR_SLOW_CPU),true)
    CFLAGS2 += -DCPUMARK=0 
else
    CFLAGS2 += -DCPUMARK=10 
endif
    LDFLAGS2 += -m64
endif
##
## arm
##
## Some of these defines may be defined automatically below:
##   ARM_VFP - VFP used;
##   HARDFP - VFP + FPU-specific calling convention;
##
ifneq (,$(findstring arm,$(TARGET_ARCH)))
    ARM_DEFINES = -DARCH_NAME="\"$(TARGET_ARCH)\""
ifneq (,$(findstring arm64,$(TARGET_ARCH)))
    CFLAGS2 += -DARCH_ARM64 -DCPUMARK=0 -ffast-math -DARM_VFP
else
    CFLAGS2 += -DARCH_ARM -DCPUMARK=0 -ffast-math
ifeq ($(FLOAT_ABI),hard)
    ARM_DEFINES += -DHARDFP -DARM_VFP
endif
endif #not arm64
ifneq (,$(findstring android,$(TARGET_OS)))
ifeq ($(TARGET_ARCH),arm)
ifeq ($(FLOAT_ABI),softfp)
    ARM_DEFINES = -DARCH_NAME="\"armv7-a\"" -DARM_VFP -march=armv7-a -mfloat-abi=softfp -mfpu=vfp
    LDFLAGS2 += -Wl,--fix-cortex-a8
else
    ARM_DEFINES = -DARCH_NAME="\"armv5te\"" -march=armv5te -mtune=xscale -msoft-float
endif
endif #arm
ifeq ($(TARGET_ARCH),arm64)
    LDFLAGS2 += -mfix-cortex-a53-835769
endif #arm64
endif #android
ifneq (,$(findstring iphone,$(TARGET_OS)))
    ARM_DEFINES = -DARCH_NAME="\"$(TARGET_ARCH)\"" -DARM_VFP -arch $(TARGET_ARCH)
    LIBTOOL_FLAGS = -static -arch_only $(TARGET_ARCH)
endif #iOS
ifneq ($(MAKE_WITHOUT_SIMD),true)
    CFLAGS2 += -ftree-vectorize
endif
endif #arm
CFLAGS2 += $(ARM_DEFINES)

##
## Handling additional MAKE_xxx options
##

##
## Linux
##
ifneq (,$(findstring linux,$(TARGET_OS)))
ifeq ($(MAKE_DYNAMIC_LIB),true)
    CFLAGS2 += -fPIC
    LDFLAGS2 += -shared -Wl,-soname,$(APPNAME).so
    FILE_EXT = .so
endif
ifeq ($(MAKE_STATIC_LIB),true)
    LDFLAGS2 += -static
    FILE_EXT = .a
endif
endif
##
## OSX
##
ifeq ($(TARGET_OS),osx)
ifeq ($(MAKE_DYNAMIC_LIB),true)
    CFLAGS2 += -fPIC
    LDFLAGS2 = -dynamiclib
    FILE_EXT = .dylib
endif
endif
##
## iOS
##
ifeq ($(TARGET_OS),iphone)
ifeq ($(MAKE_STATIC_LIB),true)
    FILE_EXT = .a
endif
endif
##
## Windows
##
ifneq (,$(findstring windows,$(TARGET_OS)))
ifneq ($(TARGET_OS),windowsce)
ifneq ($(MAKE_WITH_CONSOLE),true)
    LDFLAGS2 = -mwindows
endif
ifeq ($(MAKE_DYNAMIC_LIB),true)
    FILE_EXT = .dll
    LDFLAGS2 += -shared -Wl,--kill-at
endif
endif
endif
##
## All systems
##
ifeq ($(MAKE_WITH_MULTITOUCH),true)
    CFLAGS2 += -DMULTITOUCH
ifneq (,$(findstring linux,$(TARGET_OS)))
    DEFAULT_LIBS += -lXi
endif
endif
ifeq ($(MAKE_WITHOUT_MAIN),true)
    CFLAGS2 += -DNOMAIN
endif
ifeq ($(MAKE_WITHOUT_WM),true)
    CFLAGS2 += -DNOWM
endif
ifeq ($(MAKE_WITHOUT_SIMD),true)
    CFLAGS2 += -DNOSIMD
endif

FINAL_CFLAGS = $(CFLAGS) $(CFLAGS2) $(SUNDOG_DEFINES) -I$(SUNDOG) -DAPPNAME="\"$(APPNAME)\""
FINAL_LDFLAGS = $(LDFLAGS) $(LDFLAGS2)
FINAL_LIBS = $(LIBS) $(DEFAULT_LIBS)

##
## Object files
##

MEM = $(SUNDOG)memory/
CMEM = $(MEM)code/
SOUND = $(SUNDOG)sound/
CSOUND = $(SOUND)code/
CORE = $(SUNDOG)core/
CCORE = $(CORE)code/
FS = $(SUNDOG)filesystem/
CFS = $(FS)code/
TM = $(SUNDOG)time/
CTM = $(TM)code/
WM = $(SUNDOG)window_manager/
CWM = $(WM)code/
UTILS = $(SUNDOG)utils/
CUTILS = $(UTILS)code/
VIDEO = $(SUNDOG)video/
CVIDEO = $(VIDEO)code/

STD_INCS = $(MEM)memory.h $(CORE)core.h $(CORE)log.h $(CORE)main.h
MAIN_OBJS = main.o
LOG_OBJS = log.o
MEMORY_OBJS = memory.o
FS_OBJS = filesystem.o
TIME_OBJS = timemanager.o
UTILS_OBJS = utils.o
VIDEO_OBJS = video.o
SOUND_INCS = \
    $(SOUND)sound.h \
    $(CSOUND)sound_common.h \
    $(CSOUND)sound_linux.h \
    $(CSOUND)sound_linux_alsa_midi.h \
    $(CSOUND)sound_jack.h \
    $(CSOUND)sound_win.h \
    $(CSOUND)sound_win_mmsystem_midi.h \
    $(CSOUND)sound_android.h \
    $(CSOUND)sound_coremidi.h \
    $(CSOUND)sound_osx.h
SOUND_OBJS = sound_player.o sound.o
WM_INCS = \
    $(CWM)handlers_dialog.h \
    $(CWM)handlers_files.h \
    $(CWM)handlers_list.h \
    $(CWM)handlers_popup.h \
    $(CWM)handlers_virtual_keyboard.h \
    $(CWM)wm_android.h \
    $(CWM)wm_win.h \
    $(CWM)wm_wince.h \
    $(CWM)wm_opengl.h \
    $(CWM)wm_win_ddraw.h \
    $(CWM)wm_unix_x11.h \
    $(CWM)wm_unix_sdl.h \
    $(WM)wmanager.h \
    $(WM)struct.h
ifneq ($(MAKE_WITHOUT_WM),true)
WM_OBJS = \
    wmanager.o \
    image.o \
    keymap.o \
    wbd.o \
    handlers.o \
    handlers_preferences.o \
    handlers_color_theme.o \
    handlers_scale.o \
    handlers_keymap.o \
    framebuffer.o \
    devrgn.o
endif
OBJS = \
    $(MEMORY_OBJS) \
    $(LOG_OBJS) \
    $(FS_OBJS) \
    $(TIME_OBJS) \
    $(UTILS_OBJS) \
    $(VIDEO_OBJS) \
    $(MAIN_OBJS) \
    $(SOUND_OBJS) \
    $(WM_OBJS)

ifeq ($(TARGET_OS),iphone)
    OBJS += sound_iphone.o
endif

ifneq (,$(findstring android,$(TARGET_OS)))
    OBJS += sundog_bridge.o
ifneq ($(MAKE_WITHOUT_MAIN),true)
    OBJS += android_native_app_glue.o
endif
endif

##
## Rules
##

ifneq (,$(findstring linux,$(TARGET_OS)))
.SECONDEXPANSION:
all: $$(OBJS)
	$(LD) $(FINAL_LDFLAGS) -o $(APPNAME)$(FILE_EXT) $(OBJS) $(FINAL_LIBS)
endif

ifeq ($(TARGET_OS),osx)
.SECONDEXPANSION:
all: $$(OBJS)
	$(LD) $(FINAL_LDFLAGS) -o $(APPNAME)$(FILE_EXT) $(OBJS) $(FINAL_LIBS)
endif

ifeq ($(TARGET_OS),iphone)
.SECONDEXPANSION:
ifeq ($(MAKE_STATIC_LIB),true)
all: $$(OBJS)
	$(LIBTOOL) $(LIBTOOL_FLAGS) -o $(APPNAME)$(FILE_EXT) $(OBJS)
endif
endif

ifneq (,$(findstring windows,$(TARGET_OS)))
ifneq ($(TARGET_OS),windowsce)
.SECONDEXPANSION:
all: $$(OBJS)
ifneq ($(MAKE_WITHOUT_WM),true)
	$(WINDRES) resources/win_res.rc win_res.o
	$(LD) $(FINAL_LDFLAGS) -o $(APPNAME)$(FILE_EXT) $(OBJS) win_res.o $(FINAL_LIBS)
else
	$(LD) $(FINAL_LDFLAGS) -o $(APPNAME)$(FILE_EXT) $(OBJS) $(FINAL_LIBS)
endif
endif
endif

ifeq ($(TARGET_OS),windowsce)
.SECONDEXPANSION:
all: $$(OBJS)
	$(WINDRES) resources/win_res.rc win_res.o
	$(LD) $(FINAL_LDFLAGS) -o $(APPNAME).exe $(OBJS) win_res.o $(FINAL_LIBS)
endif

main.o: $(CCORE)main.cpp $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CCORE)main.cpp

sound_player.o: $(CSOUND)sound_player.cpp $(SOUND)sound.h $(SOUND_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CSOUND)sound_player.cpp

sound.o: $(CSOUND)sound.cpp $(SOUND)sound.h $(SOUND_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CSOUND)sound.cpp

timemanager.o: $(CTM)timemanager.cpp $(TM)timemanager.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CTM)timemanager.cpp

filesystem.o: $(CFS)filesystem.cpp $(CFS)filesystem_file_type.h $(FS)filesystem.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CFS)filesystem.cpp

utils.o: $(CUTILS)utils.cpp $(UTILS)utils.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CUTILS)utils.cpp

log.o: $(CCORE)log.cpp
	$(CXX) $(FINAL_CFLAGS) -c $(CCORE)log.cpp

memory.o: $(CMEM)memory.cpp $(MEM)memory.h $(CORE)core.h
	$(CXX) $(FINAL_CFLAGS) -c $(CMEM)memory.cpp

video.o: $(CVIDEO)video.cpp $(CVIDEO)video_linux.h $(VIDEO)video.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CVIDEO)video.cpp

wmanager.o: $(CWM)wmanager.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)wmanager.cpp

image.o: $(CWM)image.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)image.cpp

keymap.o: $(CWM)keymap.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)keymap.cpp

wbd.o: $(CWM)wbd.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)wbd.cpp

handlers.o: $(CWM)handlers.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)handlers.cpp

handlers_preferences.o: $(CWM)handlers_preferences.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)handlers_preferences.cpp

handlers_color_theme.o: $(CWM)handlers_color_theme.cpp $(WM_INCS) $(STD_INCS)
	$(CC) $(FINAL_CFLAGS) -c $(CWM)handlers_color_theme.cpp

handlers_scale.o: $(CWM)handlers_scale.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)handlers_scale.cpp

handlers_keymap.o: $(CWM)handlers_keymap.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)handlers_keymap.cpp

framebuffer.o: $(CWM)framebuffer.cpp $(WM_INCS) $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(CWM)framebuffer.cpp

devrgn.o: $(WM)regions/devrgn.cpp $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(WM)regions/devrgn.cpp

ifneq (,$(findstring android,$(TARGET_OS)))
android_native_app_glue.o: $(NDK)/sources/android/native_app_glue/android_native_app_glue.c
	$(CXX) $(FINAL_CFLAGS) -c $(NDK)/sources/android/native_app_glue/android_native_app_glue.c
sundog_bridge.o: $(SUNDOG)various/android/sundog_bridge.cpp
	$(CXX) $(FINAL_CFLAGS) -c $(SUNDOG)various/android/sundog_bridge.cpp
endif

ifeq ($(TARGET_OS),iphone)
sound_iphone.o: $(CSOUND)sound_iphone.mm $(SOUND)sound.h $(SOUND_INCS)
	$(CMM) $(FINAL_CFLAGS) -c $(CSOUND)sound_iphone.mm
endif

clean:
	rm -f *.o *.a *.optrpt
