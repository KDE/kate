try
{
    ${cursor}
}
/*%- if 0 < catch_count -%*/
    /*%- for num in range(0, catch_count) -%*/
/*< nl >*/catch (const ${exception_/*< num >*/}&)
{
}
    /*%- endfor -%*/
/*%- endif -%*/
/*%- if catch_all -%*/
/*< nl >*/catch (...)
{
}/*< nl >*/
/*%- endif -%*/
//# kate: hl C++
