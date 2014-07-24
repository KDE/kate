/*%- if template_params -%*/
template </*% for tp in template_params %*//*% if not loop.first %*/, /*% endif %*/typename /*<tp | editable >*//*% endfor %*/>
/*%- else -%*/
template <${tparams:typename T}>
/*%- endif -%*/
//# kate: hl C++
