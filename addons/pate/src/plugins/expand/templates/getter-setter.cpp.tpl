/*% if use_reference %*/const /*< member_type | editable >*/&
/*%- else -%*/
/*< member_type | editable(name='member_type') >*//*% endif %*/ /*< getter_name | editable(name='getter_name') >*/() const
{
    return /*< member_name | editable(active=True) >*/;
}
/*% if setter_returns_self %*/${self_type}/*% else %*/void/*% endif %*/ /*< setter_name | editable(name='setter_name') >*/(/*< member_type | editable(name='member_type') >*/ value)
{
    /*< member_name | editable >*/ = value;
    /*%- if setter_returns_self %*/
    return *this
    /*%- endif %*/
}
//# kate: hl C++
