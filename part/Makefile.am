SUBDIRS = interfaces part data plugins
EXTRA_DIST = AUTHORS COPYING.LIB ChangeLog NEWS README TODO Doxyfile

messages:
	$(EXTRACTRC) ./*/*.rc >> ./rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/katepart.pot
