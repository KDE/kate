SUBDIRS = part data scripts plugins tests
EXTRA_DIST = AUTHORS COPYING.LIB ChangeLog NEWS README TODO

messages:
	$(EXTRACTRC) ./*/*.rc >> rc.cpp
	$(EXTRACTATTR) --attr=language,name,Language --attr="language,section,Language Section" data/*.xml >> rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` part/*.h -o $(podir)/katepart.pot

DOXYGEN_REFERENCES = kdecore dcop kio kdeui kparts
include ../admin/Doxyfile.am

