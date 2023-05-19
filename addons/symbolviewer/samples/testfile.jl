# You may find the following packages useful:
# • Traceur.jl
# • Infiltrator.jl
#

using Parameters, Unitful, Catalyst, Documenter, DocStringExtensions

"""Superclass for parameters to allow a common set of methods:
• display
• pairs
• params2dict
"""
abstract type AbstractParameters end

"""Boltzmann formulae for voltage-dependent acivation and inactivation of a voltage-dependent ion channel
$(TYPEDSIGNATURES)
These are, effectively, logistic functions (particular case of sigmoid functions

See also Boltzmann_inact

# Arguments

• v:  Membrane voltage

• V½: Voltage at half-maximal activation or inactivation

• 𝒌:  'slope' factor

# Description

More correctly, these are Maxwell-Boltzmann equations for two possible states in the system:

p₁/(p₁+p₂) = exp(-E₁/(𝒌T)) / (exp(-E₁/(𝒌T)) + exp(-E₂/(𝒌T))) where

Eⱼ = ½mv² + Uⱼ

m  = particle mass

v  = particle velocity

Uⱼ = potential energy for state 𝒋

𝒌  = 'slope factor'

They model the steady-state activation (or inactivation) e.g. m∞ and h∞, 
and their respective time contants (τₘ, τₕ, ...), which relate to the
forward and backward reaction rates 

αₘ → forward (e.g. C → O)


=> p₁ = 1/(1+exp(-(E₂-E₁)/(𝒌T)))

For cell membranes E is free energy expressed in voltage:

p₁ = 1/(1+exp(-(V₂-V₁)/(𝒌T)))

**NOTE:** that for Nernst formula the slope factor is parameterized as 

𝒌 = 𝑹T / (𝒛𝑭) with 𝑹 the gas constant, 𝑭 Faraday's constant, and 𝒛 ionic valence.



# Examples:

```julia
v = collect(-100.0:1.0:100.0); # membrane voltage from -100 to +100 mV in 1 mV increment

# using fitted parameters in Magee & Johnston 1995
act = CTModels.Boltzmann_act.(v, -30, 7.2);

inact = CTModels.Boltzmann_inact.(v, -62, 6.9);

# NOTE: Below, we use 'mm' from Plots.PlotMeasures as fully qualified;
# less comfortable for typing, but avoids clashes with Catalyst.mm() 
# Michaelis-Menten rate function and Unitful.DefaultSymbols.mm
plot(v, inact, lc=:blue, label="inact", legend_position=:bottomright, right_margin=15.0Plots.PlotMeasures.mm, ylabel="Normalized current", xlabel="Membrane potential (mV)", xlim=(-100.0, 0.0))

plot!(twinx(), v, act, lc=:red, label="activation", legend_position=:topright, ylabel="Normalized conductance", xlim=(-100.0, 0.0))
```

"""
Boltzmann_act(v, V½, 𝒌) = 1/(1+exp((V½-v)/𝒌))


"""Method for the `display` function in Base module → α σ ∑ ↴ ↸ 💻 ᴴ ˘  ̀a """
Base.display(x::AbstractParameters) = disp(x)

"""Method for the `pairs` function in Base module.
Returns pairs of key/values from a parameters struct"""
Base.pairs(x::AbstractParameters; strip::Bool=false) = ifelse(strip,
                                                           map((f) -> f => stripUnits(getproperty(x, f)), fieldnames(typeof(x))),\
                                                           map((f) -> f => getproperty(x, f), fieldnames(typeof(x))),
                                                          )

"""Nicer display IMHO """
function disp(x::AbstractParameters)
    str = string(typeof(x)) * ':' * "\n"
    longestFieldName = maximum(length, map((x) -> string(x), fieldnames(typeof(x))))
    fields = sort(collect(fieldnames(typeof(x))))
    for field in fields
        str = str * "  " * ':' * string(field) * " "^(longestFieldName - length(string(field))) * " => " * string(getproperty(x, field)) * "\n"
    end
    
    print(str)
end

function func_1(v::Union{Int64, Float64};
                extraParam::Bool = false)
    # some non-sensical code
    if useHH
        α = inline_terse_func_1(v)
    else
        α = inline_terse_func_2(v)
    end
    β = no_inline_terse_func_1(v)
    τ  = max(0.5 / (α + β), 0.02)  # time constant of gating variable m
    x∞ = α / (α + β)
    
    return x∞, α, β, τ
end

@inline function inline_func_1(v::Union{Int64, Float64})
    return = 0.4   * (v + 30) / (1 - exp(-(v + 30)/7.2))
end

@inline function inline_func_2(α{T},bᵢ{T}, ∂{T}) where T
end

@inline function inline_func_3(α{T},bᵢ{T}, 
                               ∂{T}) where T
end

"""Some comments here for the next function"""
@inline inline_terse_func_1(v::Union{Int64, Float64}) = 0.1   * (v + 40) / (1 - exp(-(v + 40)/10)) 
@inline inline_terse_func_2(v::{T}) where T = 0.124 * (v + 30) / (exp((v+30)/7.2) - 1)
@inline inline_terse_func_3(v::Union{Int64, Float64}, c::Any) = c   * (v + 40) / (1 - exp(-(v + 40)/10)) 


"""Type factory for new AbstractParameters with fields from different 
AbstractParameters subtypes.

# Signatures

$(TYPEDSIGNATURES)
"""
"""
Merge distinct `AbstractParameterStruct` subtypes into a new `AbstractParameterStruct` type.

# Signatures

$(TYPEDSIGNATURES)
# Details

The function generates a new Struct-like DataType named `typename` and its 
custom unpacking macro entitled `@unpack_typename` in the namespace of the 
CTModels module.

The new DataType and the customized unpacking macro are then exported into the 
namespace of the Main module (so they are accessible directly from REPL).

The parameter structs passed to the function in the `paramStructs` argument must
either inherit from AbstractParameterStruct (if theye are instances) or be a 
subtype of AbstractParameterStruct (if they are types). Failing that, the function 
throws an error.

In addition, the parameter structs to be merged must be disjoint: i.e., they must
have no fields in common - the presence of duplicate fields in the resulting type
throws an error.

RETURNS:

`nothing`

The function creates a new data type

**WARNING:** 
DO NOT bind the returned value to the same symbol as the one passed as first 
argument! Doing so will assign `nothing` to the same symbol as the one used to
bind the newly created data type - effectively, nullifies the result!

In other words, given p0, p1, p2, p3, p4 instances of DISTINCT `AbstractParameterStruct`
structures,

To create a MyParams struct DO call:

```julia
mergeParameters(:MyParams, p0, p1, p2, p3, p4)
```

but DO NOT call:

```julia
MyParams = mergeParameters(:MyParams, p0, p1, p2, p3, p4)
```

(by assigning the return to the same symbol as the one used for the new type,
you overwrite i.e. nullify the new type just created: the symbol will be bound
to `nothing` instead of the type you wanted to create!)

To use the new type, just call its constructor. 
• Without arguments, the (default)constructor will generate a new instance of the
new struct type just created, where the fields have their default values.

• To create a new instance of the new struct type with CUSTOM field values, pass
key/value pairs (symbol/new value) only for those fields you want to have custom
values.

To display the fields in MyParams (useful to create a new instance of this type):

```julia
fieldnames(MyParams)
```

To find out what data types are expected for these fields:
WARNING: Due to current code limitations (read: BUG) the type specification of 
the original field (when the original field is a Unitful.Quantity) is LOST in 
the merged struct.

As a workaround, you can infer the expected types by calling ```fieldnames(...)```
as above.

```julia
fieldtypes(MyParams)
```

Finally, you can ```unpack``` the fields in the new struct type in a function, 
e.g.:

```julia
function test_me(p::MyParams)
    @unpackParameters p
end
```

**WARNING**
Note, however, that this code construct is "static" i.e., you MUST know the name
of the new struct type at the time you write the function using it !

# Examples:

```julia

# create the ParametersType1 custom type
@with_kw struct ParametersType1 <: AbstractParameterStruct
    a::Int64 = 1
    b::Float64 = 2.3
end

# create an instance of ParametersType1 with default values
params1 = ParametersType1() 

# create another custom type:
@with_kw struct ParametersType2 <: AbstractParameterStruct
    c = "abc"
    d = 1u"μm"
end

# create an instance of ParametersType2 with default values
params2 = ParametersType2()

# create a new custom type with the field merged from ParametersType1 and
# ParametersType2

mergeParameters(:MyNewParametersType, params1, params2)

# instantiate the new type with its default values

merged_params = MyNewParametersType()

# unpack ALL its members at REPL
# => all the fields are copied to variables with the same 'names' in the 
# namespace of the Main module
@unpack_MyNewParametersType merged_params

a+b
# outputs 3.3

# unpack ALl the fields inside a function
# => all the fields will be directly available inside the function

function myfunction(p::MyNewParametersType)
    @unpack_MyNewParametersType p 
    return a + b
end

myfunction(merged_params)
# outputs 3.3
    
```

To later modify the value of one of the fields of an instance of MyNewParametersType
use the `reconstruct` function (reexported from Parameters):

```julia
merged_params2 = reconstruct(merged_params, <fieldName>=new_value)
```

where `<fieldName>` is the name of an existing field in `merged_params`


"""
function mergeParameters(typename::Symbol, paramStructs...)
    if length(paramStructs) == 0
        return nothing
    end
    p0 = paramStructs[1]
    if !isa(p0, AbstractParameters)
        @error "Expecting an instance of an AbstractParameters subtype"
    end
    f0 = str(p0)
    
    for p in paramStructs[2:end]
        if !isa(p, AbstractParameters)
            @error "Expecting an instance of an AbstractParameters subtype"
        end
        pf = str(p)
        push!(f0, pf...)
    end
    
    ff = join(f0, "; ")
    prog = "@with_kw struct $typename <: AbstractParameters "*ff*" end; export $typename , @unpack_$typename";
    
    expr = Meta.parse(prog) # ⋆ for formal use
    return eval(expr) # ⋆ for normal use; for debugging, use any of the next two lines
    # return eval(expr), expr # for debugging ONLY; use the starred lines for normal use
    # return expr # for exclusive debugging ONLY; use the starred lines for normal use
    # return prog # for exclusive debugging ONLY; use the starred lines for normal use
    
end

"""function with parameters on multiple lines (and no line continuation character)"""
function mutating_func!(xdot, x, p, t::{T}) where T

    C, γ𝓁, E𝓁, t₀, t₁, Iinj, uᵥ, uₜ, uᵢ = p;
    
    v = x[1]
    vv = v * uᵥ  
    
    v_  = stripUnits(vv, u"mV")
    
    tₜ = t * uₜ
    
    Iₑ = tₜ ≥ t₀ && tₜ < t₁ ? Iinj : zero(Iinj)
    
    I𝓁  = γ𝓁  * (vv -E𝓁)
    Iₜ  = I𝓁 + Iₑ   
    ∂v = stripUnits(Iₜ/C, uᵥ/uₜ)

    xdot[1] = ∂v # dimensionless
    
end

@noinline function no_inline_func!(a,b,c)
end

@noinline no_inline_terse_func_1(v::Union{Int64, Float64}) = 0.03  * (v + 45) / (1 - exp(-(v+45)/1.5))   # => CAUTION: NaN when v = -45
@noinline no_inline_terse_func_2(a,b,c) =  a + b

"""Creates an OrderedDict from a parameters struct"""
params2dict(x::AbstractParameters) = OrderedDict(sort(collect(type2dict(x)), by=(y)->y[1]))

"""Returns a string representation of a subtype of AbstractParameters, or of 
an instance of AbstractParameters subtype.
"""
function str(p::Union{AbstractParameters, DataType})
    ff = if p isa AbstractParameters 
        return collect(map((x) -> isa(getproperty(p,x), Quantity) ? String(x)*" = "*str(getproperty(p, x)) : String(x)*" = "*str(getproperty(p,x)), fieldnames(typeof(p))))
    elseif supertype(p) == AbstractParameters # p is a Type so we instantiate it here with default properties
        pp = p()
        return collect(map((x) -> String(x)*"::Unitful.Quantity = "*str(getproperty(pp, x)), fieldnames(p))) # collect default values
    else
        @error "Expecting an AbstractParameters subtype or an instance of it"
        
    end
end

    
tersefunc(a,b) = a+b


println("ENa = ", ENa, " EK = ", EK, " E𝓁 = ", E𝓁, " useMonitor = ", useMonitor)

"""Foolin' around with metaprogramming in Julia - DON'T USE !!!

$(TYPEDSIGNATURES)


"""
macro unpackParameters(pstruct)
    # field symbols -> this, eval'ed at REPL, resolves to the list of symbols in :pstruct
    # the symbols and obkjects bound to them become available in the caller
    # namespace
    fexp = Expr(:call, fieldnames, Expr(:call, typeof, :($pstruct)))
    ret = quote
        local val = $pstruct
        local fsyms = $fexp
        eval(Expr(:block, [:($f=$val.$f) for f in fsyms]...))
    end

    return esc(ret)
end
    
struct Data1
end


@with_kw struct Data2 <: AbstractParameters
    D::Unitful.Length{Float64} = 30.0u"μm"
    z::Int64 = 1
    bᵢ::Float64 = 0.8
    temperature::Unitful.Temperature{Float64} = 35.0u"°C"
    timespan::NTuple{2, Unitful.Time{Float64}} = (0.0u"ms", 500.0u"ms")
end

mutable struct MutableData1
end

@with_kw mutable struct MutableData2
end

