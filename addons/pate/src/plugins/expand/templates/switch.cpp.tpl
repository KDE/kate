switch (/*% if expression %*//*< expression >*//*% else %*/${expression}/*% endif %*/)
{/*<nl>*/
/*%- if cases -%*/
/*%- for case in cases -%*/
case /*% if editable_cases -%*/
    /*< case | editable(name=case) >*/
/*%- else -%*/
    /*< case >*/
/*%- endif -%*/:/*%- if loop.first -%*/${cursor}/*% endif %*/
    break;/*<nl>*/
/*%- endfor -%*/
/*%- else -%*/
case ${case_1}:
    break;/*<nl>*/
/*%- endif -%*/
/*%- if default %*/
default:
    break;/*<nl>*/
/*%- endif -%*/
}
//# kate: hl C++
