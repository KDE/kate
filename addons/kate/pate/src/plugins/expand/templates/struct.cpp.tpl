//% extends 'basic-entity.cpp.tpl'
//% block declaration
/*%- if template and not template_params -%*/
/*< nl >*/template <${tparams:typename T}>
/*%- elif template and template_params -%*/
/*< nl >*/template <
    /*%- for tp in template_params -%*/
        /*%- if not loop.first %*/, /*% endif %*/typename /*<tp | editable >*/
    /*%- endfor %*/>
/*%- endif %*/
/*< entity_type | lower >*/ /*< entity_name | editable(active=True) >*/
{
    ${cursor}
};/*% endblock %*/
//# kate: hl C++; remove-trailing-spaces false;
