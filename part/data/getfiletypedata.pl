#!/usr/bin/perl

open CONFIG, ">./katefiletypesrc" || die "couldn't open for writing: $!\n";

opendir DIR, "." || die "couldn't open directory: $!\n";
@files = readdir( DIR );
closedir ( DIR );
FILES: for ( @files ) {
  next unless /\.xml$/;
  print "getting data from $_\n";
  open FILE, $_ || die "error opening file '$_': $!\n";
  my ( $inlangtag, $name, $section, $wildcards);
  while (<FILE>) {
    $inlangtag++ if /<language/;
    if ( $inlangtag ) {
      $inlangtag-- if />/;
      /\bname="(\w+)"/i and $name = $1;
      /\bsection="(\w+)"/i and $section = $1;
      /\bextensions="([^"]+)"/i and $wildcards = $1;
      /\bpriority="(\d+)"/i and $priority = $1;
      /\bmimetype="([^"]+)"/i and $mimetype = $1;
    }
    if ( $name && ! $inlangtag ) {
      ### print some stuff to a files
      print CONFIG <<"END";
[$name]
Active=true
Highlighting=$name
Mimetypes=$mimetype
Priority=$priority
Section=$section
Wildcards=$wildcards

END
      close FILE;
      next FILES;
    }
  }
}

close CONFIG;