switch(/*% if expression %*//*< expression >*//*% else %*/${expression}/*% endif %*/)
{
/*%- if cases -%*/
/*%- for case in cases %*/
case /*< case >*/:/*%- if loop.first -%*/${cursor}/*% endif %*/
    break;
/*%- endfor -%*/
/*%- else %*/
case ${case_1}:
    break;
/*% endif %*/
/*%- if default %*/
default:
    break;
/*%- endif %*/
}
//# kate: hl C++11
