ifeq ($(CC), gcc)
CFLAGS+=	-Wall -Wshadow -std=c99 -pedantic
endif

ifeq ($(CC), clang)
CFLAGS+=	-Wall -std=c99 -pedantic -Wunused-macros
endif

ifeq ($(CC), icc)
CFLAGS+=	-w2 -wd2259,2557,869,981 -std=c99
endif

ifeq ($(CC), suncc)
CFLAGS+=	-xc99=all -Xc -v
endif

ifeq ($(CC), opencc)
CFLAGS+=	-Wall -std=c99
endif

ifeq ($(CC), cparser)
CFLAGS+=	-Wno-attribute -std=c99 --strict
endif

ifeq ($(CC), ccc-analyzer)
CFLAGS+=	-std=c99
endif

ifneq ($(CCHOST),)
CC:=	$(CCHOST)-$(CC)
endif
