SUBDIRS = interfaces part data plugins
EXTRA_DIST = AUTHORS COPYING.LIB ChangeLog NEWS README TODO 

messages:
	$(EXTRACTRC) ./*/*.rc >> ./rc.cpp
	perl -n kate-extract-i18n-xml.pl data/*.xml >> rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` part/*.h -o $(podir)/katepart.pot

DOXYGEN_REFERENCES = kdecore dcop kio kdeui kparts
include ../admin/Doxyfile.am

