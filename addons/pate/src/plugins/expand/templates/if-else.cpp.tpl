if (${cond})/*%- if br %*/
{
    ${cursor}
}/*%- else -%*/${cursor}/*%- endif -%*/
/*%- if 1 < else_count -%*/
/*%- for e in range(0, else_count) %*/
else if (${cond/*< e >*/})/*% if br %*/
{
}/*%- else -%*//*< nl >*//*%- endif -%*/
/*%- endfor -%*/
/*%- endif -%*/
/*%- if else_count %*/
else/*%- if br %*/
{
}/*%- endif -%*/
/*%- endif -%*/
//# kate: hl C++
