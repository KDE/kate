if(${cond})
#%- if want_else %#
    ${cursor}
else(#% if duplicate_condition %#${cond}#% endif -%#)
    #% else %#
    ${cursor}
#%- endif %#
endif(#% if duplicate_condition %#${cond}#% endif %#)
### kate: hl cmake
