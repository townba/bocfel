ifeq ($(CC), gcc)
COMPILER_FLAGS=	-Wall -Wshadow -Wmissing-prototypes -std=c11 -pedantic
endif

ifeq ($(CC), clang)
COMPILER_FLAGS=	-Wall -Wunused-macros -Wmissing-prototypes -std=c11 -pedantic
endif

ifeq ($(CC), icc)
COMPILER_FLAGS=	-Wall -std=c11
endif

ifeq ($(CC), suncc)
COMPILER_FLAGS=	-std=c11 -pedantic
endif

ifeq ($(CC), tcc)
COMPILER_FLAGS=	-std=c11
endif

ifeq ($(CC), cparser)
COMPILER_FLAGS=	-Wno-experimental -Wno-unused-parameter -std=c11
endif

ifeq ($(shell basename $(CC)), ccc-analyzer)
COMPILER_FLAGS=	-std=c11
endif

ifneq ($(CCHOST),)
CC:=	$(CCHOST)-$(CC)
endif
