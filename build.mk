# Team: FIX Connectivity & Simulation

name = cppactor
type = static
lang = cpp

source = source/actor.cpp \
		 source/pool_base.cpp \
		 source/framework.cpp \
		 source/message.cpp

cpp_compiler_flags += -Wno-unused-parameter        \
                      -Wno-type-limits             \
                      -Wno-switch                  \
                      -Wno-delete-non-virtual-dtor \
                      -Wno-strict-aliasing         \
                      -Wno-uninitialized 

no_pedantic = 0

include_paths = . \
	include  \
	../misc/cppactor/include \
	../miscutils/include \
    ../../the_arsenal/ttstl/include \
    ../logger/include \

static_libs = miscutils
shared_libs = ttlogger
libraries = 
generation_dep = 
