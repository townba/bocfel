ifeq ($(CXX), g++)
COMPILER_FLAGS=	-Wall -Wshadow -Wswitch -Wno-sign-compare -std=c++14 -pedantic
endif

ifeq ($(CXX), clang++)
COMPILER_FLAGS=	-Wall -Wunused-macros -std=c++14 -pedantic
endif

ifeq ($(CXX), icpc)
COMPILER_FLAGS=	-Wall -std=c++14
endif

ifeq ($(CXX), icpx)
COMPILER_FLAGS=	-Wall -std=c++14
endif

ifeq ($(CXX), sunCC)
COMPILER_FLAGS=	-std=c++14 -pedantic
endif

ifeq ($(CXXHOST),)
REALCXX:=	$(CXX)
else
REALCXX:=	$(CXXHOST)-$(CXX)
endif
