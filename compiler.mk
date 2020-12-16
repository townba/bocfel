ifeq ($(CC), gcc)
COMPILER_FLAGS=	-Wall -Wshadow -Wmissing-prototypes -std=c99 -pedantic
endif

ifeq ($(CC), clang)
COMPILER_FLAGS=	-Wall -Wunused-macros -Wmissing-prototypes -Wno-tautological-constant-out-of-range-compare -std=c99 -pedantic
endif

ifeq ($(CC), icc)
COMPILER_FLAGS=	-w2 -ww1793 -wd2259,2557,869,981 -std=c99
endif

ifeq ($(CC), suncc)
COMPILER_FLAGS=	-xc99=all -Xc -v
endif

ifeq ($(CC), opencc)
COMPILER_FLAGS=	-Wall -std=c99
endif

ifeq ($(CC), cparser)
COMPILER_FLAGS=	-Wno-experimental -std=c99
endif

ifeq ($(shell basename $(CC)), ccc-analyzer)
COMPILER_FLAGS=	-std=c99
endif

ifneq ($(CCHOST),)
CC:=	$(CCHOST)-$(CC)
endif
