SRCS=	blorb.cpp branch.cpp dict.cpp iff.cpp io.cpp mathop.cpp meta.cpp memory.cpp objects.cpp osdep.cpp patches.cpp process.cpp random.cpp screen.cpp sound.cpp stack.cpp stash.cpp unicode.cpp util.cpp zoom.cpp zterp.cpp
OBJS=	$(SRCS:%.cpp=%.o)

CXXFLAGS=	-g

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
    SRCS+=	glkstart.cpp
    CXXFLAGS+=	-I$(GLK)
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

    ifeq ($(GLKSTARTUP),)
    ifeq ($(GLK), winglk)
        GLKSTARTUP=	windows
    else
        GLKSTARTUP=	unix
    endif
    endif

    ifeq ($(GLKSTARTUP), unix)
        MACROS+=	-DZTERP_GLK_UNIX
    else ifeq ($(GLKSTARTUP), windows)
        MACROS+=	-DZTERP_GLK_WINGLK
    else
        $(error Unknown Glk startup: $(GLKSTARTUP))
    endif
endif

ifeq ($(PLATFORM), unix)
    MACROS+=	-DZTERP_UNIX
ifndef GLK
ifndef NO_CURSES
    LDADD+=	-lcurses
endif
endif
ifdef ICU
    MACROS+=	-DZTERP_ICU
    CXXFLAGS+=	$(shell pkg-config icu-uc --cflags)
    LDADD+=	$(shell pkg-config icu-uc --libs)
endif
endif

ifeq ($(PLATFORM), win32)
    MACROS+=	-DZTERP_WIN32
endif

ifeq ($(PLATFORM), dos)
    MACROS+=	-DZTERP_DOS
endif

ifdef NO_CURSES
    MACROS+=	-DZTERP_NO_CURSES
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

%.o: %.cpp
ifdef V
	$(REALCXX) $(OPT) $(CXXFLAGS) $(COMPILER_FLAGS) $(MACROS) -c $<
else
	@echo $(REALCXX) $(OPT) $(CXXFLAGS) $(MACROS) -c $<
	@$(REALCXX) $(OPT) $(CXXFLAGS) $(COMPILER_FLAGS) $(MACROS) -c $<
endif

bocfel: $(OBJS)
	$(REALCXX) $(OPT) -o $@ $^ $(LDADD)

clean:
	rm -f bocfel *.o

install: bocfel
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)
	install bocfel $(DESTDIR)$(BINDIR)
	install -m644 bocfel.6 $(DESTDIR)$(MANDIR)

.PHONY: depend
depend:
	makedepend -f- -Y $(MACROS) $(SRCS) > deps 2> /dev/null

bocfel.html: bocfel.6
	mandoc -Thtml -Ostyle=mandoc.css,toc -Ios= $< > $@

include deps
