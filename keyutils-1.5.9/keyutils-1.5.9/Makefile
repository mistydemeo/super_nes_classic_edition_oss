CPPFLAGS	:= -I.
CFLAGS		:= -g -Wall -Werror
INSTALL		:= install
DESTDIR		:=
SPECFILE	:= keyutils.spec
NO_GLIBC_KEYERR	:= 0
NO_ARLIB	:= 0
ETCDIR		:= /etc
BINDIR		:= /bin
SBINDIR		:= /sbin
SHAREDIR	:= /usr/share/keyutils
MANDIR		:= /usr/share/man
MAN1		:= $(MANDIR)/man1
MAN3		:= $(MANDIR)/man3
MAN5		:= $(MANDIR)/man5
MAN7		:= $(MANDIR)/man7
MAN8		:= $(MANDIR)/man8
INCLUDEDIR	:= /usr/include
LNS		:= ln -sf

###############################################################################
#
# Determine the current package version from the specfile
#
###############################################################################
vermajor	:= $(shell grep "%define vermajor" $(SPECFILE))
verminor	:= $(shell grep "%define verminor" $(SPECFILE))
MAJOR		:= $(word 3,$(vermajor))
MINOR		:= $(word 3,$(verminor))
VERSION		:= $(MAJOR).$(MINOR)

TARBALL		:= keyutils-$(VERSION).tar
ZTARBALL	:= $(TARBALL).bz2

###############################################################################
#
# Determine the current library version from the version script
#
###############################################################################
libversion	:= $(filter KEYUTILS_%,$(shell grep ^KEYUTILS_ version.lds))
libversion	:= $(lastword $(libversion))
libversion	:= $(lastword $(libversion))
APIVERSION	:= $(subst KEYUTILS_,,$(libversion))
vernumbers	:= $(subst ., ,$(APIVERSION))
APIMAJOR	:= $(firstword $(vernumbers))

ARLIB		:= libkeyutils.a
DEVELLIB	:= libkeyutils.so
SONAME		:= libkeyutils.so.$(APIMAJOR)
LIBNAME		:= libkeyutils.so.$(APIVERSION)

###############################################################################
#
# Guess at the appropriate lib directory and word size
#
###############################################################################
ifeq ($(origin LIBDIR),undefined)
LIBDIR		:= $(shell ldd /usr/bin/make | grep '\(/libc\)' | sed -e 's!.*\(/.*\)/libc[.].*!\1!')
endif
ifeq ($(origin USRLIBDIR),undefined)
USRLIBDIR	:= $(patsubst /lib/%,/usr/lib/%,$(LIBDIR))
endif
BUILDFOR	:= $(shell file /usr/bin/make | sed -e 's!.*ELF \(32\|64\)-bit.*!\1!')-bit

LNS		:= ln -sf

ifeq ($(origin CFLAGS),undefined)
ifeq ($(BUILDFOR),32-bit)
CFLAGS		+= -m32
LIBDIR		:= /lib
USRLIBDIR	:= /usr/lib
else
ifeq ($(BUILDFOR),64-bit)
CFLAGS		+= -m64
LIBDIR		:= /lib64
USRLIBDIR	:= /usr/lib64
endif
endif
endif

###############################################################################
#
# This is necessary if glibc doesn't know about the key management error codes
#
###############################################################################
ifeq ($(NO_GLIBC_KEYERR),1)
CFLAGS	+= -DNO_GLIBC_KEYERR
LIBLIBS	:= -ldl -lc
else
LIBLIBS	:=
endif

###############################################################################
#
# Normal build rule
#
###############################################################################
all: $(DEVELLIB) keyctl request-key key.dns_resolver

###############################################################################
#
# Build the libraries
#
###############################################################################
#RPATH = -Wl,-rpath,$(LIBDIR)

ifeq ($(NO_ARLIB),0)
all: $(ARLIB)
$(ARLIB): keyutils.o
	$(AR) rcs $@ $<
endif

VCPPFLAGS	:= -DPKGBUILD="\"$(shell date -u +%F)\""
VCPPFLAGS	+= -DPKGVERSION="\"keyutils-$(VERSION)\""
VCPPFLAGS	+= -DAPIVERSION="\"libkeyutils-$(APIVERSION)\""

keyutils.o: keyutils.c keyutils.h Makefile
	$(CC) $(CPPFLAGS) $(VCPPFLAGS) $(CFLAGS) -UNO_GLIBC_KEYERR -o $@ -c $<


$(DEVELLIB): $(SONAME)
	ln -sf $< $@

$(SONAME): $(LIBNAME)
	ln -sf $< $@

LIBVERS := -shared -Wl,-soname,$(SONAME) -Wl,--version-script,version.lds

$(LIBNAME): keyutils.os version.lds Makefile
	$(CC) $(CFLAGS) -fPIC $(LDFLAGS) $(LIBVERS) -o $@ keyutils.os $(LIBLIBS)

keyutils.os: keyutils.c keyutils.h Makefile
	$(CC) $(CPPFLAGS) $(VCPPFLAGS) $(CFLAGS) -fPIC -o $@ -c $<

###############################################################################
#
# Build the programs
#
###############################################################################
%.o: %.c keyutils.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

keyctl: keyctl.o $(DEVELLIB)
	$(CC) -L. $(CFLAGS) $(LDFLAGS) $(RPATH) -o $@ $< -lkeyutils

request-key: request-key.o $(DEVELLIB)
	$(CC) -L. $(CFLAGS) $(LDFLAGS) $(RPATH) -o $@ $< -lkeyutils

key.dns_resolver: key.dns_resolver.o $(DEVELLIB)
	$(CC) -L. $(CFLAGS) $(LDFLAGS) $(RPATH) -o $@ $< -lkeyutils -lresolv

###############################################################################
#
# Install everything
#
###############################################################################
install: all
ifeq ($(NO_ARLIB),0)
	$(INSTALL) -D -m 0644 $(ARLIB) $(DESTDIR)$(USRLIBDIR)/$(ARLIB)
endif
	$(INSTALL) -D $(LIBNAME) $(DESTDIR)$(LIBDIR)/$(LIBNAME)
	$(LNS) $(LIBNAME) $(DESTDIR)$(LIBDIR)/$(SONAME)
	mkdir -p $(DESTDIR)$(USRLIBDIR)
	$(LNS) $(LIBDIR)/$(SONAME) $(DESTDIR)$(USRLIBDIR)/$(DEVELLIB)
	$(INSTALL) -D keyctl $(DESTDIR)$(BINDIR)/keyctl
	$(INSTALL) -D request-key $(DESTDIR)$(SBINDIR)/request-key
	$(INSTALL) -D request-key-debug.sh $(DESTDIR)$(SHAREDIR)/request-key-debug.sh
	$(INSTALL) -D key.dns_resolver $(DESTDIR)$(SBINDIR)/key.dns_resolver
	$(INSTALL) -D -m 0644 request-key.conf $(DESTDIR)$(ETCDIR)/request-key.conf
	mkdir -p $(DESTDIR)$(ETCDIR)/request-key.d
	mkdir -p $(DESTDIR)$(MAN1)
	$(INSTALL) -m 0644 $(wildcard man/*.1) $(DESTDIR)$(MAN1)
	mkdir -p $(DESTDIR)$(MAN3)
	$(INSTALL) -m 0644 $(wildcard man/*.3) $(DESTDIR)$(MAN3)
	mkdir -p $(DESTDIR)$(MAN5)
	$(INSTALL) -m 0644 $(wildcard man/*.5) $(DESTDIR)$(MAN5)
	mkdir -p $(DESTDIR)$(MAN7)
	$(INSTALL) -m 0644 $(wildcard man/*.7) $(DESTDIR)$(MAN7)
	mkdir -p $(DESTDIR)$(MAN8)
	$(INSTALL) -m 0644 $(wildcard man/*.8) $(DESTDIR)$(MAN8)
	$(LNS) keyctl_describe.3 $(DESTDIR)$(MAN3)/keyctl_describe_alloc.3
	$(LNS) keyctl_get_security.3 $(DESTDIR)$(MAN3)/keyctl_get_security_alloc.3
	$(LNS) keyctl_instantiate.3 $(DESTDIR)$(MAN3)/keyctl_instantiate_iov.3
	$(LNS) keyctl_instantiate.3 $(DESTDIR)$(MAN3)/keyctl_reject.3
	$(LNS) keyctl_instantiate.3 $(DESTDIR)$(MAN3)/keyctl_negate.3
	$(LNS) keyctl_instantiate.3 $(DESTDIR)$(MAN3)/keyctl_assume_authority.3
	$(LNS) keyctl_link.3 $(DESTDIR)$(MAN3)/keyctl_unlink.3
	$(LNS) keyctl_read.3 $(DESTDIR)$(MAN3)/keyctl_read_alloc.3
	$(LNS) recursive_key_scan.3 $(DESTDIR)$(MAN3)/recursive_session_key_scan.3
	$(INSTALL) -D -m 0644 keyutils.h $(DESTDIR)$(INCLUDEDIR)/keyutils.h

###############################################################################
#
# Run tests
#
###############################################################################
test:
	$(MAKE) -C tests run

###############################################################################
#
# Clean up
#
###############################################################################
clean:
	$(MAKE) -C tests clean
	$(RM) libkeyutils*
	$(RM) keyctl request-key key.dns_resolver
	$(RM) *.o *.os *~
	$(RM) debugfiles.list debugsources.list

distclean: clean
	$(RM) -r rpmbuild

###############################################################################
#
# Generate a tarball
#
###############################################################################
$(ZTARBALL):
	git archive --prefix=keyutils-$(VERSION)/ --format tar -o $(TARBALL) HEAD
	bzip2 -9 $(TARBALL)

tarball: $(ZTARBALL)

###############################################################################
#
# Generate an RPM
#
###############################################################################
SRCBALL	:= rpmbuild/SOURCES/$(TARBALL)

BUILDID	:= .local
dist	:= $(word 2,$(shell grep "%dist" /etc/rpm/macros.dist))
release	:= $(word 2,$(shell grep ^Release: $(SPECFILE)))
release	:= $(subst %{?dist},$(dist),$(release))
release	:= $(subst %{?buildid},$(BUILDID),$(release))
rpmver	:= $(VERSION)-$(release)
SRPM	:= rpmbuild/SRPMS/keyutils-$(rpmver).src.rpm

RPMBUILDDIRS := \
		--define "_srcrpmdir $(CURDIR)/rpmbuild/SRPMS" \
		--define "_rpmdir $(CURDIR)/rpmbuild/RPMS" \
		--define "_sourcedir $(CURDIR)/rpmbuild/SOURCES" \
		--define "_specdir $(CURDIR)/rpmbuild/SPECS" \
		--define "_builddir $(CURDIR)/rpmbuild/BUILD" \
		--define "_buildrootdir $(CURDIR)/rpmbuild/BUILDROOT"

RPMFLAGS := \
	--define "buildid $(BUILDID)"

rpm:
	mkdir -p rpmbuild
	chmod ug-s rpmbuild
	mkdir -p rpmbuild/{SPECS,SOURCES,BUILD,BUILDROOT,RPMS,SRPMS}
	git archive --prefix=keyutils-$(VERSION)/ --format tar -o $(SRCBALL) HEAD
	bzip2 -9f $(SRCBALL)
	rpmbuild -ts $(SRCBALL).bz2 --define "_srcrpmdir rpmbuild/SRPMS" $(RPMFLAGS)
	rpmbuild --rebuild $(SRPM) $(RPMBUILDDIRS) $(RPMFLAGS)

rpmlint: rpm
	rpmlint $(SRPM) $(CURDIR)/rpmbuild/RPMS/*/keyutils-{,libs-,libs-devel-,debuginfo-}$(rpmver).*.rpm

###############################################################################
#
# Build debugging
#
###############################################################################
show_vars:
	@echo VERSION=$(VERSION)
	@echo APIVERSION=$(APIVERSION)
	@echo LIBDIR=$(LIBDIR)
	@echo USRLIBDIR=$(USRLIBDIR)
	@echo BUILDFOR=$(BUILDFOR)
	@echo SONAME=$(SONAME)
	@echo LIBNAME=$(LIBNAME)
