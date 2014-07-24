//% extends 'basic-entity.cpp.tpl'
//% block declaration
/*%- if template and not template_params -%*/
/*< nl >*/template <${tparams:typename T}>
/*%- elif template and template_params -%*/
/*< nl >*/template <
/*%- for tp in template_params -%*/
    /*%- if not loop.first -%*/, /*% endif -%*/typename /*<tp | editable >*/
/*%- endfor %*/>
/*%- elif template_full_spec -%*/
/*< nl >*/template <>
/*%- endif %*/
/*< entity_type | lower >*/ /*< entity_name | editable(active=True) >*//*%- if template_full_spec -%*/<${specialization-params}>/*% endif %*/
{
public:
/*% if default_ctor -%*/
    /// Default constructor
    /*< entity_name | editable >*/()/*% if default_ctor is string %*/ = /*< default_ctor >*/;/*< nl >*/
    /*%- else %*/
    {
    }/*< nl >*/
    /*%- endif -%*/
/*%- endif %*/
/*%- if 1 <= ctor_arity -%*/
    /// Make an instance using given /*% if ctor_arity == 1 %*/parameter/*% else %*/parameters/*% endif %*/
    /*% if ctor_arity == 1 %*/explicit /*% endif %*//*< entity_name | editable >*/(/*% for i in range(ctor_arity) %*//*% if not loop.first%*/, /*% endif %*/${param/*<i>*/}/*% endfor %*/)
    {
    }/*< nl >*/
/*%- endif -%*/
/*% if default_copy_ctor -%*/
    /// Copy constructor
    /*< entity_name | editable >*/(const /*< entity_name | editable >*/&)/*% if default_copy_ctor is string %*/ = /*< default_copy_ctor >*/;/*% else %*/
    {
    }
    /*%- endif -%*/
/*< nl >*/    /// Copy assign
    /*< entity_name | editable >*/& operator=(const /*< entity_name | editable >*/&)/*% if default_copy_ctor is string %*/ = /*< default_copy_ctor >*/;/*< nl >*//*% else %*/
    {
        return *this;
    }/*< nl >*/
    /*%- endif -%*/
/*%- endif %*/
/*%- if default_move_ctor -%*/
    /// Move constructor
    /*< entity_name | editable >*/(/*< entity_name | editable >*/&&)/*% if default_move_ctor is string %*/ = /*< default_move_ctor >*/;/*% else %*/
    {
    }
    /*%- endif -%*/
/*< nl >*/    /// Move assign
    /*< entity_name | editable >*/& operator=(/*< entity_name | editable >*/&&)/*% if default_move_ctor is string %*/ = /*< default_move_ctor >*/;/*< nl >*//*% else %*/
    {
        return *this;
    }/*< nl >*/
    /*%- endif -%*/
/*%- endif %*/
/*%- if destructor -%*/
    /// Destroy an instance of /*< entity_name | editable >*/
    /*% if virtual_destructor %*/virtual /*% endif %*/~/*< entity_name | editable >*/()
    {
    }/*< nl >*/
/*%- endif -%*/
};/*% endblock %*/
//# kate: hl C++; remove-trailing-spaces false;
