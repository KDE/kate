#!/usr/local/bin/perl -w
# little script to extract the text from the .xml files
# and output it, so xgettext can add the tips to the po file
#
# 2000 by Matthias Kiefer <matthias.kiefer@gmx.de>

if($#ARGV<0) {
    die "Usage: perl preparedata.pl *.xml\n";
}

open(OUT,">data.cpp") || die "Unable to open the output file (data.cpp)";
print OUT "// This file is generated on make messages and it makes it possible\n";
print OUT "// to translate the strings in the XML files in the data directory.\n\n";

foreach(@ARGV) {
    open(FILE,"<$_") || die "Unable to open ", $_;
    while(<FILE>) {
       if(m/.*language\s+name\s*=\s*\"(.*?)\"/) {
            print OUT "i18n( \"", $1, "\" );\n";
        }
        if(m/.*language\s+name\s*=\s*\"(.*?)\"\s+section\s*=\s*\"(.*?)\"/) {
            print OUT "i18n( \"", $2, "\" );\n";
        }
        if(m/.*itemData\s+name\s*=\s*\"(.*?)\"/) {
            print OUT "i18n( \"", $1, "\" );\n";
        }
    }
    close(FILE);
}
print OUT "\n"; # final newline :)
close(OUT);
