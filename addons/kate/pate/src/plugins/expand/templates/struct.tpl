//% extends 'basic-entity.tpl'
//% block declaration
/*% if template and not template_params %*/
template <${tparams:typename T}>
/*% elif template and template_params %*/
template </*% for tp in template_params %*//*% if not loop.first %*/, /*% endif %*/typename /*<tp | editable >*//*% endfor %*/>
/*%- endif %*/
/*< entity_type | lower >*/ /*< entity_name | editable(active=True) >*/
{
    ${cursor}
};/*% endblock %*/
//# kate: hl C++11; remove-trailing-spaces false;
