include Src.mk

B2D_BASE=vender/Box2D_v2.2.1/
B2D_SRC=\
	$(wildcard $(B2D_BASE)/Box2D/Collision/*.cpp) \
	$(wildcard $(B2D_BASE)/Box2D/Collision/Shapes/*.cpp) \
	$(wildcard $(B2D_BASE)/Box2D/Common/*.cpp) \
	$(wildcard $(B2D_BASE)/Box2D/Dynamics/*.cpp) \
	$(wildcard $(B2D_BASE)/Box2D/Dynamics/Joints/*.cpp) \
	$(wildcard $(B2D_BASE)/Box2D/Dynamics/Contacts/*.cpp) \
	$(wildcard $(B2D_BASE)/Box2D/Rope/*.cpp)

CPP_SRC+=$(GAME_SRC) \
	$(B2D_SRC)

LUA_BASE=vender/lua-5.2.1/src
LUA_SRC=\
	$(LUA_BASE)/lapi.c $(LUA_BASE)/lauxlib.c \
	$(LUA_BASE)/lbaselib.c $(LUA_BASE)/lbitlib.c \
	$(LUA_BASE)/lcode.c $(LUA_BASE)/lcorolib.c \
	$(LUA_BASE)/lctype.c $(LUA_BASE)/ldblib.c \
	$(LUA_BASE)/ldebug.c $(LUA_BASE)/ldo.c \
	$(LUA_BASE)/ldump.c $(LUA_BASE)/lfunc.c \
	$(LUA_BASE)/lgc.c $(LUA_BASE)/linit.c \
	$(LUA_BASE)/liolib.c $(LUA_BASE)/llex.c \
	$(LUA_BASE)/lmathlib.c $(LUA_BASE)/lmem.c \
	$(LUA_BASE)/loadlib.c $(LUA_BASE)/lobject.c \
	$(LUA_BASE)/lopcodes.c $(LUA_BASE)/loslib.c \
	$(LUA_BASE)/lparser.c $(LUA_BASE)/lstate.c \
	$(LUA_BASE)/lstring.c $(LUA_BASE)/lstrlib.c \
	$(LUA_BASE)/ltable.c $(LUA_BASE)/ltablib.c \
	$(LUA_BASE)/ltm.c $(LUA_BASE)/lundump.c \
	$(LUA_BASE)/lvm.c $(LUA_BASE)/lzio.c

TREMOR_SRC=$(wildcard vender/tremor/*.c)
OGG_SRC=$(wildcard vender/libogg-1.3.0/src/*.c)

OGG_HEADER=vender/libogg-1.3.0/include/ogg/config_types.h

C_SRC+=sfmt/SFMT.c spectra.c $(OGG_SRC) $(TREMOR_SRC) $(LUA_SRC) stb_image.c

CFLAGS+=-Isfmt/ -Ivender/tremor/ -Ivender/libogg-1.3.0/include/ -Ivender/lua-5.2.1/src
CXXFLAGS+=$(CFLAGS) -std=c++0x -U__STRICT_ANSI__ -Wno-invalid-offsetof -I$(B2D_BASE)

LDFLAGS+=-lpthread
C_OBJS=\
	$(patsubst %.cpp,%.o,$(CPP_SRC)) \
	$(patsubst %.c,%.o,$(C_SRC))

EXE_OBJS+=$(C_OBJS)

all: $(OGG_HEADER) $(BINS)

%.bin: %.o $(EXE_OBJS) $(LUA_LIB)
	$(CXX) -o $@ $< $(EXE_OBJS) $(LDFLAGS) $(CXXFLAGS)

test_bin: $(C_OBJS) testlib_test.o
	$(CXX) -o $@ $(C_OBJS) testlib_test.o $(LDFLAGS) $(CXXFLAGS)

spriter_test: $(C_OBJS) spriter_test.o
	$(CXX) -o $@ $(C_OBJS) spriter_test.o $(LDFLAGS) $(CXXFLAGS)

test: test_bin
	./test_bin

$(OGG_HEADER): vender/libogg-1.3.0/configure
	cd vender/libogg-1.3.0 ; $(EMCONFIGURE) ./configure

C_TOOL_OBJS=$(C_OBJS)

sfmt/SFMT.o: sfmt/SFMT.c
	$(CXX) $(CXXFLAGS) -c $< -o $@ -DSFMT_MEXP=607

GAME_OBJS=$(patsubst %.cpp,%.o,$(GAME_SRC))

clean:
	rm -rf *.o $(BIN) buildatlas test items_bin

distclean: clean
	rm -rf $(C_OBJS)
	(cd vender/libogg-1.3.0 ; make distclean)
	(cd vender/lua-5.2.1 ; make clean)

.phony: all pngs
