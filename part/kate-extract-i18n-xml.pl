#

#
# Copyright (C) 2004 Nicolas GOUTTE <goutte@kde.org>
# License: LGPL 2.0
#

sub writei18n
{
    $_ = @_[1];
    if ( $_ )
    {
        s/&lt;/</g;
        s/&gt;/>/g;
        s/&apos;/\'/g;
        s/&quot;/\"/g;
        s/&amp;/&/g;
        s/\"/\\\"/g;
        print "i18n( \"". @_[0] . "\", \"" . $_ . "\" );\n";
    }
}

if ( m/<language (.+)>/ )
{
    $attributes = $1;
    #print "// " . $attributes . "\n";

    $attributes =~ m/name=\"(.+?)\" /;
    writei18n( "Language", $1 );
    
    $attributes =~ m/section=\"(.+?)\" /;
    writei18n( "Language Section", $1 );
    
}