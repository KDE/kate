/*%- if namespaces -%*/
/*% for ns in namespaces %*//*% if not loop.first %*/ /*% endif %*/namespace /*< ns >*/ {/*% endfor %*/
${cursor}
/*< close_braces >*//*< padding >*/// namespace
/*%- for ns in namespaces | reverse -%*/
    /*%- if loop.first %*/ /*% else %*/, /*% endif %*//*< ns >*/
/*%- endfor -%*/
/*%- else -%*/
namespace {
${cursor}
}                                                           // anonymous namespace
/*%- endif -%*/
//# kate: hl C++
