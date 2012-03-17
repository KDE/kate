# Copyright 2011 Alex Turbov <i.zaufi@gmail.com>
#
# Make a list of namespaces in two arrays
#

{
    j = NF - 1;
    br = "";
    for (i = 1; i <= NF; i++)
    {
        printf "namespace[%i]=%s;\n", i - 1, $i;
        printf "reverse_namespace[%i]=%s;\n", j--, $i;
        br = br "}";
    }
    printf "ns-close=\"%-60s\";", br
}
