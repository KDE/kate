//# kate: hl C++
/*%- if template -%*/template </*%- for p in range(0, template_arity) -%*//*%- if not loop.first -%*/, /*% endif -%*/typename ${T/*%- if 1 < template_arity -%*//*< p >*//*%- endif -%*/}/*%- endfor -%*/>/*< nl >*//*%- endif -%*/
/*< inline >*//*< static >*//*< virtual >*/${void} ${name}(/*%- if 0 < arity -%*/
/*%- for p in range(0, arity) -%*//*%- if not loop.first -%*/, /*% endif -%*/${param/*< p >*/}/*%- endfor -%*/
/*%- endif -%*/)/*< const >*//*< final >*//*< override >*//*< pure >*//*%- if br %*/
{
    ${cursor}
}/*%- else -%*/;/*%- endif -%*/
