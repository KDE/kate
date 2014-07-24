/*%- if for_type == 'auto' -%*/
/*% if br -%*/
for (/*%- if const -%*/const /*% endif -%*/${type:auto}/*< ref >*/ ${name:item} : /*< var | editable(name='var') >*/)
{
    ${cursor}
}
/*%- else -%*/
for (/*%- if const -%*/const /*% endif -%*/${type:auto}/*< ref >*/ ${name:item} : /*< var | editable(name='var') >*/)${cursor}
/*%- endif -%*/
/*%- elif for_type == 'iter' -%*/
for (
/*%- if for_sub_type == 'c++03' %*/
    ${type:/*%- if const -%*/const_/*% endif -%*/iterator} ${name:it} = /*< var | editable(name='var') >*/.begin()
  , ${last} = /*< var | editable(name='var') >*/.end()
/*%- else %*///# NOTE Default subtype is C++11 w/ ADL
    ${type:auto} ${name:it} = /*% if for_sub_type == 'c++11' -%*/std::/*%- endif -%*/begin(/*< var | editable(name='var') >*/)
  , ${last} = /*% if for_sub_type == 'c++11' -%*/std::/*%- endif -%*/end(/*< var | editable(name='var') >*/)
/*%- endif %*/
  ; ${name:it} != ${last}
  ; ++${name:it}
  )
{
    ${cursor}
}
/*%- else -%*/
for (${type:auto} /*< var | editable(name='var') >*/ = ${start:0}; /*< var | editable(name='var') >*/ != ${last}; ++i){$cursor}
/*%- endif -%*///# kate: hl C++
