DEBUG = NDEBUG

VK_VER = 1.4.335.0

CXX = g++
CXXFLAGS = -std=c++23 -D$(DEBUG) -DVK_NO_PROTOTYPES -Wall
CXXFLAGS += -Wno-unknown-pragmas -Wno-parentheses -Wno-unused-result -Wno-unused-variable
SPVASFLAGS = --target-env=vulkan1.2
INCLUDE = -I../Include/ -IInclude -I../$(VK_VER)/x86_64/include/
LIB = -L../$(VK_VER)/x86_64/lib/ -ldl -lvolk

OBJS = ../Lib/General.o ../Lib/PhysDevice.o ../Lib/Shader.o

ifeq ($(DEBUG), _DEBUG)
	CXXFLAGS += -O0 -g
else
	CXXFLAGS += -O3 -march=icelake-server -LTO -Winline
endif

.PHONY: Witness

Witness: $(OBJS)
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c Witness/Witness.cpp -o Witness/Witness.o
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c Witness/Main.cpp -o Witness/Main.o
	$(CXX) $(CXXFLAGS) Witness/Witness.o Witness/Main.o $(OBJS) $(LIBS) -o Witness/witness
