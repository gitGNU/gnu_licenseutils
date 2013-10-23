# Copyright (C) 2013 Ben Asselstine
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
ACLOCAL_AMFLAGS = -I lib/m4 -I m4 
SUBDIRS=lib src po tests
EXTRA_DIST = bootstrap bootstrap.conf TODO build-aux/config.rpath \
	     HACKING completion/licensing lush.1

if USE_BASH_COMPLETION
  bash_completion_DATA = completion/licensing
else
  bash_completion_DATA = 
endif

# the following makery is to facilitate the creation of a tarball
# that can be built using the ./bootstrap command.
PREBOOTSTRAPFILES=\
$(srcdir)/configure.ac \
$(srcdir)/styles.ac \
$(srcdir)/Makefile.am \
$(srcdir)/bootstrap \
$(srcdir)/bootstrap.conf \
$(srcdir)/README \
$(srcdir)/AUTHORS \
$(srcdir)/COPYING \
$(srcdir)/NEWS \
$(srcdir)/TODO \
$(srcdir)/HACKING \
$(srcdir)/INSTALL \
$(srcdir)/ChangeLog \
$(srcdir)/lush.1 \
$(srcdir)/po/POTFILES.in \
$(srcdir)/po/LINGUAS \
$(srcdir)/completion/licensing \
$(srcdir)/src/Makefile.am \
$(srcdir)/src/styles.am \
$(srcdir)/src/licensing.c      $(srcdir)/src/licensing.h \
$(srcdir)/src/licensing_priv.h \
$(srcdir)/src/gettext-more.h \
$(srcdir)/src/util.h            $(srcdir)/src/util.c \
$(srcdir)/src/gpl.c             $(srcdir)/src/gpl.h \
$(srcdir)/src/lgpl.c            $(srcdir)/src/lgpl.h \
$(srcdir)/src/agpl.c            $(srcdir)/src/agpl.h \
$(srcdir)/src/fdl.c             $(srcdir)/src/fdl.h \
$(srcdir)/src/help.c            $(srcdir)/src/help.h \
$(srcdir)/src/welcome.c         $(srcdir)/src/welcome.h \
$(srcdir)/src/warranty.c        $(srcdir)/src/warranty.h \
$(srcdir)/src/boilerplate.c     $(srcdir)/src/boilerplate.h \
$(srcdir)/src/copyright.c       $(srcdir)/src/copyright.h \
$(srcdir)/src/cbb.c             $(srcdir)/src/cbb.h \
$(srcdir)/src/comment.c         $(srcdir)/src/comment.h \
$(srcdir)/src/uncomment.c       $(srcdir)/src/uncomment.h \
$(srcdir)/src/opts.c            $(srcdir)/src/opts.h \
$(srcdir)/src/choose.c          $(srcdir)/src/choose.h \
$(srcdir)/src/top.c             $(srcdir)/src/top.h \
$(srcdir)/src/project.c         $(srcdir)/src/project.h \
$(srcdir)/src/extra.c           $(srcdir)/src/extra.h \
$(srcdir)/src/all-permissive.c  $(srcdir)/src/all-permissive.h \
$(srcdir)/src/png-boilerplate.c $(srcdir)/src/png-boilerplate.h \
$(srcdir)/src/png-apply.c       $(srcdir)/src/png-apply.h \
$(srcdir)/src/bsd.c             $(srcdir)/src/bsd.h \
$(srcdir)/src/mit.c             $(srcdir)/src/mit.h \
$(srcdir)/src/isc.c             $(srcdir)/src/isc.h \
$(srcdir)/src/apache.c          $(srcdir)/src/apache.h \
$(srcdir)/src/apply.c           $(srcdir)/src/apply.h \
$(srcdir)/src/prepend.c         $(srcdir)/src/prepend.h \
$(srcdir)/src/c-style.c         $(srcdir)/src/c-style.h \
$(srcdir)/src/c++-style.c       $(srcdir)/src/c++-style.h \
$(srcdir)/src/fortran-style.c   $(srcdir)/src/fortran-style.h \
$(srcdir)/src/pascal-style.c    $(srcdir)/src/pascal-style.h \
$(srcdir)/src/gettext-style.c   $(srcdir)/src/gettext-style.h \
$(srcdir)/src/groff-style.c     $(srcdir)/src/groff-style.h \
$(srcdir)/src/haskell-style.c   $(srcdir)/src/haskell-style.h \
$(srcdir)/src/m4-style.c        $(srcdir)/src/m4-style.h \
$(srcdir)/src/scheme-style.c    $(srcdir)/src/scheme-style.h \
$(srcdir)/src/shell-style.c     $(srcdir)/src/shell-style.h \
$(srcdir)/src/texinfo-style.c   $(srcdir)/src/texinfo-style.h \
$(srcdir)/src/styles.c          $(srcdir)/src/styles.h \
$(srcdir)/src/comment-style.h \
$(srcdir)/src/main.c \
$(srcdir)/src/lush.in \
$(srcdir)/src/new-boilerplate.in \
$(srcdir)/src/preview.in \
$(srcdir)/src/notice.in \
$(srcdir)/tests/Makefile.am \
$(srcdir)/tests/boilerplate/Makefile.am \
$(srcdir)/tests/boilerplate/0001 \
$(srcdir)/tests/boilerplate/0002 \
$(srcdir)/tests/cbb/Makefile.am \
$(srcdir)/tests/cbb/0001 \
$(srcdir)/tests/choose/Makefile.am \
$(srcdir)/tests/choose/0001 \
$(srcdir)/tests/choose/0002 \
$(srcdir)/tests/comment/Makefile.am \
$(srcdir)/tests/comment/0001 \
$(srcdir)/tests/copyright/Makefile.am \
$(srcdir)/tests/copyright/0001 \
$(srcdir)/tests/prepend/Makefile.am \
$(srcdir)/tests/prepend/0001 \
$(srcdir)/tests/project/Makefile.am \
$(srcdir)/tests/project/0001 \
$(srcdir)/tests/top/Makefile.am \
$(srcdir)/tests/top/0001 \
$(srcdir)/tests/uncomment/Makefile.am \
$(srcdir)/tests/uncomment/0001 \
$(srcdir)/tests/uncomment/0002 \
$(srcdir)/tests/uncomment/0003

manifest:
	@echo ${PREBOOTSTRAPFILES}

.manifest:
	make srcdir="`eval basename ${PWD}`" --silent -f ${PWD}/Makefile manifest > ${builddir}/.manifest

predist: ${builddir}/.manifest
	rm -f ${PACKAGE}.tar.gz 2>/dev/null ;\
	tar --dereference --directory=.. -zcvf ${PACKAGE}.tar.gz `cat ${builddir}/.manifest` && rm ${builddir}/.manifest

CLEANFILES=.manifest licensing.1

#this chunk of makery is about getting a man page to build,
#despite a horribly broken help2man, and a non-standard --help in licensing.
man1_MANS=licensing.1 lush.1

licensing.1: $(builddir)/src/licensing
	$(HELP2MAN) --output=$(builddir)/$@ --name='a program for adding license notices to files' --source licenseutils --libtool  $(builddir)/src/licensing


