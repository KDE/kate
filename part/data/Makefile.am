kde_services_DATA = katepart.desktop

rckatedir = $(kde_datadir)/katepart
rckate_DATA = katepartui.rc katepartreadonlyui.rc

syntaxkatedir = $(kde_datadir)/katepart/syntax
syntaxkate_DATA = language.dtd syntax.template cpp.xml c.xml html.xml kbasic.xml objectivec.xml \
                  changelog.xml ada.xml css.xml perl.xml php.xml xml.xml java.xml rpmspec.xml \
                sql.xml sql-mysql.xml sql-postgresql.xml vhdl.xml diff.xml bash.xml latex.xml postscript.xml ruby.xml \
                desktop.xml eiffel.xml pascal.xml sather.xml python.xml makefile.xml \
                ferite.xml scheme.xml matlab.xml tcl.xml ilerpg.xml verilog.xml javascript.xml \
                sml.xml winehq.xml cs.xml sgml.xml idconsole.xml xmldebug.xml fortran.xml haskell.xml \
                literate-haskell.xml r.xml gnuassembler.xml mab.xml mason.xml idl.xml prolog.xml \
                rsiidl.xml coldfusion.xml picsrc.xml uscript.xml lpc.xml velocity.xml sci.xml ldif.xml \
                inform.xml xharbour.xml yacas.xml component-pascal.xml asm6502.xml gdl.xml bibtex.xml d.xml \
                vrml.xml rexx.xml cue.xml progress.xml e.xml sieve.xml modula-2.xml awk.xml cisco.xml \
                doxygen.xml lua.xml pike.xml debianchangelog.xml debiancontrol.xml fgl-4gl.xml fgl-per.xml alert.xml \
                gettext.xml commonlisp.xml clipper.xml povray.xml euphoria.xml mup.xml jsp.xml ocaml.xml lex.xml yacc.xml \
                cg.xml html-php.xml css-php.xml javascript-php.xml ahdl.xml ansic89.xml tibasic.xml \
                purebasic.xml mips.xml logtalk.xml txt2tags.xml xslt.xml stata.xml glsl.xml \
                lilypond.xml abc.xml asp.xml asm-avr.xml rib.xml cmake.xml octave.xml javadoc.xml cgis.xml \
                spice.xml nasm.xml mediawiki.xml apache.xml m3u.xml ini.xml pgn.xml freebasic.xml ansys.xml

partrcdir   = $(kde_confdir)
partrc_DATA = katesyntaxhighlightingrc katefiletyperc

html-php.xml: $(srcdir)/html.xml $(srcdir)/generate-php.pl
	$(PERL) $(srcdir)/generate-php.pl < $(srcdir)/html.xml > $@

css-php.xml: $(srcdir)/css.xml $(srcdir)/generate-php.pl
	$(PERL) $(srcdir)/generate-php.pl < $(srcdir)/css.xml > $@

javascript-php.xml: $(srcdir)/javascript.xml $(srcdir)/generate-php.pl
	$(PERL) $(srcdir)/generate-php.pl < $(srcdir)/javascript.xml > $@
