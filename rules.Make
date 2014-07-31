#
# Information below should not be changed
#


SILENT	= -s
PRINTDIR = --no-print-directory
#MAKE     = /usr/bin/make $(PRINTDIR) $(SILENT)
MAKE = make


LIBDIR=/usr/local/lib 
#DEPDIR = .deps


CC	= gcc
CXX	= g++
AR	= ar
LD	= ld


INCS = \
	-I /usr/local/include \
	-I /usr/include




#PROFILE = -pg
CFLAGS   = $(EXTRA_INCS) $(INCS) $(EXTRA_CFLAGS)  
CXXFLAGS = $(EXTRA_INCS) $(INCS) $(EXTRA_CXXFLAGS) 

CFLAGS   += -mtune=generic -Wall -Werror -fPIC -fno-strict-aliasing -fno-unit-at-a-time -fms-extensions
CXXFLAGS += -mtune=generic -Wall -Werror -fno-strict-aliasing -fno-unit-at-a-time -fms-extensions -fPIC 

CFLAGS += -g 
#CFLAGS += -pg 

# clear some compile flags in some case
ifdef	CFLAGS_FILTER
CFLAGS := $(filter-out $(CFLAGS_FILTER), $(CFLAGS))
endif

ifdef	CXXFLAGS_FILTER
CXXFLAGS := $(filter-out $(CXXFLAGS_FILTER), $(CXXFLAGS))
endif

LDFLAGS = \
	-rdynamic \
        -L$(LIBDIR) \
        -Wl,-R$(LIBDIR) \
	$(EXTRA_LDFLAGS)

LIBS += $(EXTRA_LIBS)

ifdef TARGET_A
TARGETS += $(TARGET_A)
endif

ifdef TARGET_O
TARGETS += $(TARGET_O)
endif

ifdef TARGET_SO
TARGETS += $(TARGET_SO)
endif


ifdef TARGET_PROG
TARGETS += $(TARGET_PROG)
endif


ifdef TARGET_TEST
TARGETS += $(TARGET_TEST)
endif


ifdef TARGET_CXXPROG
TARGETS += $(TARGET_CXXPROG)
endif

ifdef TARGET_CXXPROG1
TARGETS += $(TARGET_CXXPROG1)
endif

ifdef TARGET_CXXPROG2
TARGETS += $(TARGET_CXXPROG2)
endif

ifdef TARGET_CXXTEST
TARGETS += $(TARGET_CXXTEST)
endif



ifdef TARGETS
#DIRTARGET= $(DEPDIR) 
endif


.PHONY: all
ifdef SUBDIRS
all: $(DIRTARGET) subdirs $(TARGETS)
else
all: $(DIRTARGET) $(TARGETS)
endif


ifdef SUBDIRS
.PHONY: subdirs

subdirs:
	@for i in $(SUBDIRS); do \
	    echo "===>compile $(PWD)/$$i"; \
	    if [ -f "$$i/Makefile" ]; then \
	        (cd $$i && $(MAKE) all) || exit 1; \
	    fi; \
	done
endif

#
# specify vpath
#
vpath %.so /usr/local/lib/
vpath %.a /usr/local/lib/ 

#
# Common built targets:
#

# TARGET_A :	OBJECTS_A
# TARGET_SO :	OBJECTS_SO
# TARGET_PROG : OBJECTS_PROG
# TARGET_TEST : OBJECTS_TEST
# TARGET_CXXPROG : OBJECTS_CXXPROG
# TARGET_CXXTEST : OBJECTS_CXXTEST

#$(DEPDIR): 
#	+@[ -d $@ ] || mkdir -p $@ 

# TARGET_A: OBJECTS_A for compiling .o files into a static library (.a file)
ifdef TARGET_A
$(TARGET_A): $(OBJECTS_A)
    ifdef SILENT
	@echo -e "\tAR\t$@"
	@$(AR) rc $@ $^
	@$(RANLIB) $@
    else
	$(AR) rc $@  $^
	$(RANLIB) $@
    endif
    ifdef LIBDIR
	-rm -rf $(LIBDIR)/$(TARGET_A)
	-mkdir -p $(LIBDIR)
	-cp $@ $(LIBDIR)/$(TARGET_A)
    endif
endif


# TARGET_O: compiling all .o files into a .o file
ifdef TARGET_O
$(TARGET_O): $(OBJECTS_O)
    ifdef SILENT
	@echo -e "\tLD\t$@"
	@$(LD) -r -o $@ $^
    else
	$(LD) -r -o $@ $^
    endif
endif


# TARGET_SO: TARGET_SO_OBJECTS for compiling .o files into a shared library (.so file)
ifdef TARGET_SO
$(TARGET_SO): $(OBJECTS_SO) 
    ifdef SILENT
	@echo -e "\tLD\t$@"
	@$(LD) -shared -soname $(TARGET_SO) -o $@ $^ $(LDDFLAGS) $(TARGET_SO_LIBS)
    else
	$(LD)  -shared -soname $(TARGET_SO) -o $@ $^ $(LDDFLAGS) $(TARGET_SO_LIBS)
    endif
    ifdef LIBDIR
	-rm -rf $(LIBDIR)/$(TARGET_SO)
	-mkdir -p $(LIBDIR)
	-cp $@ $(LIBDIR)/$(TARGET_SO)
    endif
endif

# TARGET_PROG: OBJECTS_PROG for compiling .o files into a executable
ifdef TARGET_PROG
$(TARGET_PROG): $(OBJECTS_PROG) 
    ifdef SILENT
	@echo -e "\tBIN\t$@"
	@$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_PROG_LIBS)
    else
	$(CC)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_PROG_LIBS)
    endif
    ifdef BINDIR
	-rm -f $(BINDIR)/$(TARGET_PROG)
	-mkdir -p $(BINDIR)
	-cp $@ $(BINDIR)/$(TARGET_PROG)
    endif
endif


# TARGET_PROG1: OBJECTS_PROG1 for compiling .o files into a executable
ifdef TARGET_PROG1
$(TARGET_PROG1): $(OBJECTS_PROG1) 
    ifdef SILENT
	@echo -e "\tBIN\t$@"
	@$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_PROG1_LIBS)
    else
	$(CC)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_PROG1_LIBS)
    endif
    ifdef BINDIR
	-rm -f $(BINDIR)/$(TARGET_PROG1)
	-mkdir -p $(BINDIR)
	-cp $@ $(BINDIR)/$(TARGET_PROG1)
    endif
endif

# TARGET_PROG2: OBJECTS_PROG2 for compiling .o files into a executable
ifdef TARGET_PROG2
$(TARGET_PROG2): $(OBJECTS_PROG2)
    ifdef SILENT
	@echo -e "\tBIN\t$@"
	@$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_PROG2_LIBS) 
    else
	$(CC)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_PROG2_LIBS)
    endif
    ifdef BINDIR
	-rm -f $(BINDIR)/$(TARGET_PROG2)
	-mkdir -p $(BINDIR)
	-cp $@ $(BINDIR)/$(TARGET_PROG2)
    endif
endif

# TARGET_TEST: OBJECTS_TEST for compiling .o files into a executable
ifdef TARGET_TEST
$(TARGET_TEST): $(OBJECTS_TEST)
    ifdef SILENT
	@echo -e "\tBIN\t$@"
	@$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_TEST_LIBS)
    else
	$(CC)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_TEST_LIBS)
    endif
endif

# TARGET_TEST1: OBJECTS_TEST1 for compiling .o files into a executable
ifdef TARGET_TEST1
$(TARGET_TEST1): $(OBJECTS_TEST1)
    ifdef SILENT
	@echo -e "\tBIN\t$@"
	@$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_TEST1_LIBS)
    else
	$(CC)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_TEST1_LIBS)
    endif
endif

# TARGET_TEST2: OBJECTS_TEST2 for compiling .o files into a executable
ifdef TARGET_TEST2
$(TARGET_TEST2): $(OBJECTS_TEST2)
    ifdef SILENT
	@echo -e "\tBIN\t$@"
	@$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_TEST2_LIBS)
    else
	$(CC)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_TEST2_LIBS)
    endif
endif



# TARGET_CXXPROG: C++ compiling .o files into a executable
ifdef TARGET_CXXPROG
$(TARGET_CXXPROG): $(OBJECTS_CXXPROG) 
    ifdef SILENT
	@echo -e "\tCXXBIN\t$@"
	@$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_CXXPROG_LIBS)
    else
	$(CXX)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_CXXPROG_LIBS)
    endif
    ifdef BINDIR
	-rm -f $(BINDIR)/$(TARGET_CXXPROG)
	-mkdir -p $(BINDIR)
	-cp $@ $(BINDIR)/$(TARGET_CXXPROG)
    endif
endif
# TARGET_CXXTEST: C++ compiling .o files into a executable
ifdef TARGET_CXXTEST
$(TARGET_CXXTEST): $(OBJECTS_CXXTEST) 
    ifdef SILENT
	@echo -e "\tCXXBIN\t$@"
	@$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_CXXTEST_LIBS)
    else
	$(CXX)  -o $@ $^ $(LDFLAGS) $(LIBS) $(TARGET_CXXTEST_LIBS)
    endif
endif


#
# Generating dependency files in .deps/ directory while compiling
#
%.o:%.c 
    ifdef SILENT
	@echo -e "\tCC\t$@"
	@$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.d)
    else
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.d)
    endif
	@cp $*.d $(DEPDIR)/$*.d
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $(DEPDIR)/$*.d;
	@-rm -f $*.d

%.o:%.cpp
    ifdef SILENT
	@echo -e "\tCXX\t$@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.d)
    else
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.d)
    endif
	@cp $*.d $(DEPDIR)/$*.d
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $(DEPDIR)/$*.d;
	@-rm -f $*.d

%.o:%.cc
    ifdef SILENT
	@echo -e "\tCXX\t$@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.d)
    else
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.d)
    endif
	@cp $*.d $(DEPDIR)/$*.d
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $(DEPDIR)/$*.d;
	@-rm -f $*.d


%.o:%.s
    ifdef SILENT
	@echo -e "\tASM\t$@"
	@$(CC) $(CFLAGS) -c $< -o $@ 
    else
	$(CC) $(CFLAGS) -c $< -o $@ 
    endif
	@cp $*.d $(DEPDIR)/$*.d
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $(DEPDIR)/$*.d;
	@-rm -f $*.d



# depclean rules
.PHONY: depclean
depclean :
ifdef	SUBDIRS
	@for i in $(SUBDIRS); do \
	    echo "===>depclean $(PWD)/$$i"; \
	    if [ -f "$$i/Makefile" ]; then \
	        (cd $$i && $(MAKE) depclean) || exit 1; \
	    fi; \
	done
	@for i in $(CLEANDIRS); do \
	    echo "===>clean $(PWD)/$$i"; \
	    if [ -f "$$i/Makefile" ]; then \
	        (cd $$i && $(MAKE) clean) || exit 1; \
	    fi; \
	done
endif
	@-rm -rf *.o *.d $(TARGETS) $(EXTRA_CLEANFILES)
	@-rm -rf $(DEPDIR)

# clean rules
.PHONE: clean
clean: 
ifdef	SUBDIRS
	@for i in $(SUBDIRS); do \
	    echo "===>clean $(PWD)/$$i"; \
	    if [ -f "$$i/Makefile" ]; then \
	        (cd $$i && $(MAKE) clean) || exit 1; \
	    fi; \
	done
	@for i in $(CLEANDIRS); do \
	    echo "===>clean $(PWD)/$$i"; \
	    if [ -f "$$i/Makefile" ]; then \
	        (cd $$i && $(MAKE) clean) || exit 1; \
	    fi; \
	done

endif
	@-rm -rf *.o *.d $(TARGETS) $(EXTRA_CLEANFILES)


#-include $(DEPDIR)/*.d
