#
#	Makefile For FortiWEB.
#

#include .config


# make options
SILENT  = -s
PRINTDIR= --no-print-directory
MAKE	= /usr/bin/make $(PRINTDIR) $(SILENT)

SUBDIRS =http

.PHONY : help config all image genversion clean depclean

.PHONY: subdirs
subdirs :  
	@for i in $(SUBDIRS); do \
		echo -e "===>compile $(PWD)/$$i"; \
		if [ -f "$$i/Makefile" ]; then \
			(cd $$i && $(MAKE) all) || exit 1; \
		fi; \
	done


config : build/genversion Configure
	./Configure


all: subdirs 



clean:
	@for i in $(SUBDIRS); do \
		echo -e "===>clean $(PWD)/$$i"; \
		if [ -f "$$i/Makefile" ]; then \
			(cd $$i && $(MAKE) clean) || exit 1; \
		fi; \
	done


depclean:
	@for i in $(SUBDIRS); do \
		echo -e "===>depclean $(PWD)$$i"; \
		if [ -f "$$i/Makefile" ]; then \
			(cd $$i && $(MAKE) depclean) || exit 1; \
		fi; \
	done
	-rm -f image/*
	-rm -f image.out
	-rm -f image.out.ovf.zip
	-rm -f image.out.vmware.zip
	-rm -f image.out.xenopensource.zip
	-rm -f image.out.xenserver.zip
	-rm -f include/autoconf.h
	-rm -f include/version.h
	-rm -rf build/*
	-rm -f tags


help:
	@echo -e "\nFortiWeb makefile system usage:\n"
	@echo -e "make help        Show help message"
	@echo -e "make config      Configure source code"
	@echo -e "make all         Compile source code and create image"
	@echo -e "make image       Create image but not compile source code"
	@echo -e "make genversion  Compile genverion tool for CM build machine script"
	@echo -e "make clean       Clean all compiled objects"
	@echo -e "make depclean    Clean all compiled objects and remove images files. deps files"
	@echo -e "\n"



.PHONY: objdirs
objdirs: 
	@mkdir -p build/bin
	@mkdir -p build/lib
	@mkdir -p build/rlib


