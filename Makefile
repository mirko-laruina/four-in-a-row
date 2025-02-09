# Compiler and flags
CC         = g++
CFLAGS     = -g -Wall -lcrypto

# Directories
OBJDIR     = build
SRCDIR     = src
BINDIR     = dist
HDRDIR     = include
DOCDIR     = doc
DOCTMPDIR  = build/doc
FOLDERS    := $(strip $(shell find $(SRCDIR) -type d -printf '%P\n'))

# List of targets
UTILS      = client/connect4 network/inet_utils network/messages network/socket_wrapper security/secure_socket_wrapper security/crypto utils/dump_buffer network/host server/user_list utils/args client/single_player client/multi_player client/server client/server_lobby security/crypto_utils utils/buffer_io
TARGETS    = client/client server/server

SRCS = $(addsuffix .cpp, $(addprefix $(SRCDIR)/,$(UTILS))) $(addsuffix .cpp, $(addprefix $(SRCDIR)/,$(TARGETS)))

# Documentation output
DOCPDFNAME = documentation.pdf
SRCPDFNAME = source_code.pdf

# Documentation config file
DOXYGENCFG = doxygen.cfg

override CFLAGS += -I $(HDRDIR)
override LDFLAGS += -lpthread

# Object files for utilities (aka libraries)
UTILS_OBJ  = $(addsuffix .o, $(addprefix $(OBJDIR)/,$(UTILS)))

# Builds only the executables: default rule
exe: $(addprefix $(BINDIR)/,$(TARGETS))

# Utilities are secondary targets
.SECONDARY: $(UTILSOBJ)

# Build targets
$(BINDIR)/%: $(OBJDIR)/%.o $(UTILS_OBJ) $(HDRDIR)/**/*.h $(HDRDIR)/*.h $(SRCDIR)/**/*.h
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(LDFLAGS)
	chmod +x $@

# Build generic .o file from .cpp file
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HDRDIR)/**/*.h $(HDRDIR)/*.h $(SRCDIR)/**/*.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

# Note that if I modify any header everything is built again
# This is not very effective but for such small project that's not an issue

# Build documentation pdf
$(DOCDIR)/$(DOCPDFNAME): $(SRCS) $(HDRDIR)/**/*.h $(HDRDIR)/*.h $(DOXYGENCFG)
	doxygen $(DOXYGENCFG)
	( cd $(DOCTMPDIR)/latex ; make )
	cp $(DOCTMPDIR)/latex/refman.pdf $(DOCDIR)/$(DOCPDFNAME)


# prepare sorted source list for source code pdf generation
# I want the header to appear right before the c source file
# the source files of client and server will be last
both = $(HDRDIR)/$(1).h $(SRCDIR)/$(1).cpp 
ALL_SOURCES = $(foreach x,$(UTILS),$(call both,$(x)))
ALL_SOURCES += logging.h
ALL_SOURCES += $(addprefix $(SRCDIR)/,$(addsuffix .cpp,$(TARGETS)))

# Build source code ps file
$(OBJDIR)/sources.ps : $(SRCDIR)/**/*.cpp $(SRCDIR)/**/*.h $(HDRDIR)/*.h
	echo $(ALL_SOURCES)
	enscript -C -fCourier9 --highlight=c -p$(OBJDIR)/sources.ps  $(ALL_SOURCES)

# Build makefile ps file
$(OBJDIR)/makefile.ps: Makefile
	enscript -C -fCourier9 --highlight=makefile -p$(OBJDIR)/makefile.ps Makefile

$(OBJDIR)/sources.pdf: $(OBJDIR)/sources.ps
	ps2pdf $(OBJDIR)/sources.ps $(OBJDIR)/sources.pdf

$(OBJDIR)/makefile.pdf: $(OBJDIR)/makefile.ps
	ps2pdf $(OBJDIR)/makefile.ps $(OBJDIR)/makefile.pdf

# Builds source code pdf file
$(DOCDIR)/$(SRCPDFNAME): $(OBJDIR)/sources.pdf $(OBJDIR)/makefile.pdf
	pdfunite $(OBJDIR)/makefile.pdf $(OBJDIR)/sources.pdf $(DOCDIR)/$(SRCPDFNAME)

# Builds everything (ecutables and documentation)
all: exe $(DOCDIR)/$(DOCPDFNAME) $(DOCDIR)/$(SRCPDFNAME) report

# clean everything
clean:
	$(RM) -r $(OBJDIR)/* $(BINDIR)/* $(DOCDIR)/$(DOCPDFNAME) $(DOCDIR)/$(SRCPDFNAME)  doc/report/report.pdf

# clean everything and then rebuild
rebuild: clean all

# just opens output documentation
doc_open: $(DOCDIR)/$(DOCPDFNAME)
	xdg-open $(DOCDIR)/$(DOCPDFNAME)

# generates documentation and opens it in default pdf viewer
doc: $(DOCDIR)/$(DOCPDFNAME) doc_open

# makes source code pdf and opens it
source: $(DOCDIR)/$(SRCPDFNAME) 
	xdg-open doc/source_code.pdf

test: exe
	@   i=0; \
		pass=0; \
		for test_script in tests/*.sh; do \
			echo -n "$$test_script ... "; \
			sh $$test_script > /dev/null 2> /dev/null; \
			if [ "$$?" -eq "0" ]; \
			then \
				echo "PASS"; \
				pass=$$(($$pass+1)); \
			else \
				echo "FAIL"; \
			fi; \
			i=$$(($$i+1)); \
		done; \
		echo "Passed $$pass out of $$i"

doc/report/report.pdf: doc/report/*.tex
	cd doc/report; latexmk -pdf report.tex

report: doc/report/report.pdf
		
help:
	@echo "all:         builds everything (both binaries and documentation)"
	@echo "clean:       deletes any intermediate or output file in build/, dist/ and doc/"
	@echo "report:      builds the report only"
	@echo "doc:         builds documentation only and opens pdf file"
	@echo "doc_open:    opens documentation pdf"
	@echo "exe:         builds only binaries"
	@echo "help:        shows this message"
	@echo "rebuild:     same as calling clean and then all"
	@echo "source:      makes source code pdf and opens it"
	@echo "test:        runs all tests defined in tests/*.sh"

# these targets aren't name of files
.PHONY: all exe clean rebuild doc_open doc help source report

# build project structure
$(shell   mkdir -p $(DOCDIR) $(addprefix $(OBJDIR)/,$(FOLDERS)) $(BINDIR)/client $(BINDIR)/server  test)
