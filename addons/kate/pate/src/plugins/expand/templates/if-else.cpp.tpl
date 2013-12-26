//# kate: hl C++11
if (${cond})/*%- if br %*/
{
    ${cursor}
}/*%- else -%*/${cursor}/*%- endif -%*/
/*%- for e in range(0, else_count) %*/
else if (${cond/*< e >*/})/*% if br %*/
{
}/*%- else -%*//*< nl >*//*%- endif -%*/
/*%- endfor -%*/
