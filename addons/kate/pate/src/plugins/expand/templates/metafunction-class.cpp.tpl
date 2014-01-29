//% extends 'basic-entity.cpp.tpl'
//% block declaration
/*% if template and not template_params %*/
template <${tparams:typename T}>
/*% elif template and template_params %*/
template <
    /*%- for tp in template_params -%*/
        /*%- if not loop.first %*/, /*% endif %*/typename /*< tp | editable >*/
    /*%- endfor %*/>
/*%- endif %*/
/*< entity_type | lower >*/ /*< entity_name | editable(active=True) >*/
{
    /*% if mfc_params -%*/
    template <
        /*%- for tp in mfc_params -%*/
            /*%- if not loop.first %*/, /*% endif %*/typename /*< tp | editable >*/
        /*%- endfor %*/>
    /*%- else -%*/
    template <${mfcparams:typename U}>
    /*%- endif %*/
    struct apply
    {
        typedef ${action:typename } type;
    };
};/*% endblock %*/
//# kate: hl C++; remove-trailing-spaces false;
