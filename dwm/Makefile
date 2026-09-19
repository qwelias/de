# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

all: dwm

.c.o:
	${CC} -c ${CFLAGS} $<

dwm.o: config.h config.mk drw.h util.h

config.h:
	cp config.def.h $@

dwm: dwm.o
	${CC} -o $@ dwm.o ${LDFLAGS}

clean:
	rm -f dwm dwm.o dwm-${VERSION}.tar.gz

dist: clean
	mkdir -p dwm-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk\
		dwm.1 util.h dwm.c dwm.png transient.c dwm-${VERSION}
	tar -cf dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rf dwm-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1
	mkdir -p ${DESTDIR}${PREFIX}/share/xsessions
	test -f ${DESTDIR}${PREFIX}/share/xsessions/dwm.desktop || cp -n dwm.desktop ${DESTDIR}${PREFIX}/share/xsessions
	chmod 644 ${DESTDIR}${PREFIX}/share/xsessions/dwm.desktop

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm\
		${DESTDIR}${MANPREFIX}/man1/dwm.1\
		${DESTDIR}${PREFIX}/share/xsessions/dwm.desktop

.PHONY: all clean dist install uninstall
