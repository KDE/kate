#!/usr/bin/perl -w

use strict;
use Tie::File;

# get params
my $file = shift;
my $major = shift;
my $minor = shift;
my $micro = shift;

# validate
my @array;
tie @array, 'Tie::File', $file or die "Unable to open given file '$file'.\n";

# replace versions
my $majorOk = 0;
my $minorOk = 0;
my $microOk = 0;
for (@array) {
    if (s/KDE_APPLICATIONS_VERSION_MAJOR "\d+"/KDE_APPLICATIONS_VERSION_MAJOR "$major"/g) {
        $majorOk = 1;
    }

    if (s/KDE_APPLICATIONS_VERSION_MINOR "\d+"/KDE_APPLICATIONS_VERSION_MINOR "$minor"/g) {
        $minorOk = 1;
    }

    if (s/KDE_APPLICATIONS_VERSION_MICRO "\d+"/KDE_APPLICATIONS_VERSION_MICRO "$micro"/g) {
        $microOk = 1;
    }
}

# error?
if ($majorOk != 1 || $minorOk != 1 || $microOk != 1) {
    die "Failed to replace some versions. Please check if the file contains the right boiler plate code:\n\n"
        ."# KDE Application Version, managed by release script\n"
        ."set (KDE_APPLICATIONS_VERSION_MAJOR \"1\")\n"
        ."set (KDE_APPLICATIONS_VERSION_MAJOR \"1\")\n"
        ."set (KDE_APPLICATIONS_VERSION_MAJOR \"1\")\n"
        ."set (KDE_APPLICATIONS_VERSION \"\${KDE_APPLICATIONS_VERSION_MAJOR}.\${KDE_APPLICATIONS_VERSION_MINOR}.\${KDE_APPLICATIONS_VERSION_MICRO}\")\n\n";
}

# be done
exit 0;
