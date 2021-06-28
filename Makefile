SRCS=	blorb.c branch.c dict.c iff.c io.c math.c meta.c memory.c objects.c osdep.c patches.c process.c random.c screen.c sound.c stack.c stash.c unicode.c util.c zoom.c zterp.c
OBJS=	$(SRCS:%.c=%.o)

CFLAGS=	-g

include config.mk
include compiler.mk

ifdef FAST
    GLK=
    PLATFORM=
    NO_SAFETY_CHECKS=1
    NO_CHEAT=1
    NO_WATCHPOINTS=1
endif

ifdef GLK
    SRCS+=	glkstart.c
    CFLAGS+=	-I$(GLK)
    MACROS+=	-DZTERP_GLK

    include $(GLK)/Make.$(GLK)
    LDADD+=	-L$(GLK) $(GLKLIB) $(LINKLIBS)

    # Windows Glk actually does something inside of glk_tick(), so force
    # it here. There is a slight inconsistency: if you don’t want
    # glk_tick() called on Win32, you must pass “GLK_TICK=” when building,
    # whereas for all other platforms, this is unneccessary, since it’s
    # only on Win32 that a value is explicitly set for GLK_TICK.
    ifeq ($(PLATFORM), win32)
        GLK_TICK=1
    endif

    ifdef GLK_TICK
        MACROS+=	-DZTERP_GLK_TICK
    endif

    ifndef GLK_NO_BLORB
        MACROS+=	-DZTERP_GLK_BLORB
    endif
endif

ifeq ($(PLATFORM), unix)
    MACROS+=	-DZTERP_UNIX
ifndef GLK
    LDADD+=	-lcurses
endif
endif

ifeq ($(PLATFORM), win32)
    MACROS+=	-DZTERP_WIN32
endif

ifeq ($(PLATFORM), dos)
    MACROS+=	-DZTERP_DOS
endif

ifdef NO_SAFETY_CHECKS
    MACROS+=	-DZTERP_NO_SAFETY_CHECKS
endif

ifdef NO_CHEAT
    MACROS+=	-DZTERP_NO_CHEAT
endif

ifdef NO_WATCHPOINTS
    MACROS+=	-DZTERP_NO_WATCHPOINTS
endif

ifdef TANDY
    MACROS+=	-DZTERP_TANDY
endif

all: bocfel

%.o: %.c
ifdef V
	$(CC) $(OPT) $(CFLAGS) $(COMPILER_FLAGS) $(MACROS) -c $<
else
	@echo $(CC) $(OPT) $(CFLAGS) $(MACROS) -c $<
	@$(CC) $(OPT) $(CFLAGS) $(COMPILER_FLAGS) $(MACROS) -c $<
endif

bocfel: $(OBJS)
	$(CC) $(OPT) -o $@ $^ $(LDADD)

clean:
	rm -f bocfel *.o

install: bocfel
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)
	install bocfel $(DESTDIR)$(BINDIR)
	install -m644 bocfel.6 $(DESTDIR)$(MANDIR)

.PHONY: depend
depend:
	makedepend -f- -Y $(MACROS) $(SRCS) > deps 2> /dev/null

tags: $(SRCS)
	ctags -I ZASSERT --c-kinds=+l $^ *.h

bocfel.html: bocfel.6
	mandoc -Thtml -Ostyle=mandoc.css,toc -Ios= $< > $@

include deps
