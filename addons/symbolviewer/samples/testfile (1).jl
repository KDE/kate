# You may find the following packages useful:
# • Traceur.jl
# • Infiltrator.jl
#

using Parameters, Catalyst, Documenter, DocStringExtensions

"""Superclass for parameters to allow a common set of methods:
• display
• pairs
• params2dict
"""
abstract type AbstractParameters end



#BEGIN NOTE: 2022-09-01 23:25:02 
# below, the *HH functions follow the Hodgkin-Huxley (HH) model (Steratt & Willshaw)
@inline alphaₕHH(v::Union{Int64, Float64}) = 0.07  * exp(-(v + 65)/20) 
@inline alphaₕMJ(v::Union{Int64, Float64}) = 0.03  * (v + 45) / (1 - exp(-(v+45)/1.5))   # => CAUTION: NaN when v = -45
@inline alphaᵢMJ(v::Union{Int64, Float64}) = exp(0.45 * (v + 60))
@inline alphaₘHH(v::Union{Int64, Float64}) = 0.1   * (v + 40) / (1 - exp(-(v + 40)/10))  # => CAUTION: NaN when v = -40 !!!
@inline alphaₘMJ(v::Union{Int64, Float64}) = 0.4   * (v + 30) / (1 - exp(-(v + 30)/7.2)) # => CAUTION: NaN when v = -30



  Base.display(x::AbstractParameters) = disp(x)


"""Naᵥ 'm' gating particle (activation) - Migliore et al 1999"""
@inline function Base.Naᵥ_m_gate (v::Union{Int64, Float64}; useHH::Bool=false) 
    
    # NOTE: 2023-02-24 13:59:20
    # Example code to plot these (this is also valid for the functions in KV.jl):
    #
    # Vₘ = Vector(range(-100., 50., 300)) # membrane potential from -100 mV to +50 mV in 0.5 mV steps
    #
    # Naαₘ = <enter your function here>. (Vₘ) # NOTE the dot notation to vectorize the function call in Julia !
    #
    #       (i.e. choose from alphaₘHH, alphaₘMJ or any other )
    #
    # then plot:
    #
    # plot(Vₘ, Naαₘ; show=true) # or plot(Vₘ, Naαₘ); gui()
    #
    # and similarly for βₘ, τₘ and for αₕ and βₕ etc. below
    #
    # When using vectors, vectorize with dot notation.
    #
    # To plot `m` itself - this is the steady-state ! - calculate then plot
    # m∞; in the ODE system functions, ∂m is approximated as (m∞ - m) / τₘ
    #
    #
    #
    # The approach is the same for ano other channels in the model.
    #
    # To apply the entire function to a vector, e.g. to call something like
    #       Naᵥ_m_gate.(Vₘ)
    #
    # • remeber that this function returns four scalars x∞, α, β, τ for one 
    #   scalar argument `v`; 
    #
    # • ⇒ when vectorizing it [ i.e., Naᵥ_m_gate.(Vₘ) ], the result will be a 
    #   vector of 4-tuples (one tuple of x∞, α, β, τ per sample in the Vₘ vector)
    #
    #   … these need collecting in a matrix:
    #
    #
    #   res_ = Naᵥ_m_gate.(Vₘ)
    #
    #   result = transpose(hcat(collect.(res_)...))
    #   
    #   what this does is to: (a) convert each tuple into a 4-element vector (`collect.(res_)`)
    #   such that res_ becomes a vector fo vectors; then (b) concatenate the vectors (`hcat(...)`)
    #   into a matrix (NOTE the splatting of the outer vector , or vector of vector)
    #
    #   You'll just have to remember what each matrix column represents.
    #
    # To save the results to disc use something like:
    # using JLD2
    # jldsave("KvDR_n_gate_" * replace(string(now()), ":"=>"_")*".jld2";KᵥDRₙ=KᵥDRₙ)
    
    
#     α = ifelse(useHH, alphaₘHH(v), alphaₘMJ(v))
#     β = ifelse(useHH, betaₘHH(v), betaₘMJ(v))
    
    if useHH
        α = alphaₘHH(v)
        β = betaₘHH(v)
    else
        α = alphaₘMJ(v)
        β = betaₘMJ(v)
    end
    # τ = 1 / (α + β)  # time constant of gating variable m
    # x∞ = α * τ # with τ as above
    # alternatively to incorporate temperature dependency, see hh.mod:
    # τ = 1/(qNa10 * (α + β)) 
    τ  = max(0.5 / (α + β), 0.02)  # time constant of gating variable m
    x∞ = α / (α + β)
#     ∂x = (x∞ - x) / τ
    
    # ∂m = αₘ * ( 1 - m) - βₘ * m
    # requires defining q10Na
    # m∞ = αₘ  * τₘ
    
    return x∞, α, β, τ
end

"""Returns pairs of key/values from a parameters struct"""
Base.pairs(x::AbstractParameters; strip::Bool=false) = ifelse(strip, 
                                                           map((f) -> f => stripUnits(getproperty(x, f)), fieldnames(typeof(x))),
                                                           map((f) -> f => getproperty(x, f), fieldnames(typeof(x))),
                                                          )

@inline betaₕHH (v::Union{Int64, Float64})  = 1.0   / (exp(-(v + 35)/10) + 1)
betaₕMJ(v::Union{Int64, Float64}) = 0.01  * (v + 45) / (exp((v+45)/1.5) - 1)    # => CAUTION: NaN when v = -45
@inline betaᵢMJ(v::Union{Int64, Float64}) = exp(0.09 * (v + 60))
@inline betaₘHH(v::Union{Int64, Float64})  = 4.0   * exp(-(v + 65)/18)
@inline betaₘMJ(v::Union{Int64, Float64})  = 0.124 * (v + 30) / (exp((v+30)/7.2) - 1)    # => CAUTION: NaN when v = -30

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

#END NOTE: 2022-09-01 23:25:02 

function dict2params_
end

dict2params__ () = 4

"""Reverse of params2dict"""
function dict2params(d::AbstractDict{Symbol, Any}, name::Symbol)
    println("not implemented")
end


"""A (better ?) display for parameters structs.
Pushed onto the Base module as `display` method for AbstractParameters"""
function disp(x::AbstractParameters)
    str = string(typeof(x)) * ':' * "\n"
    longestFieldName = maximum(length, map((x) -> string(x), fieldnames(typeof(x))))
    fields = sort(collect(fieldnames(typeof(x))))
    for field in fields
        str = str * "  " * ':' * string(field) * " "^(longestFieldName - length(string(field))) * " => " * string(getproperty(x, field)) * "\n"
    end
    
    print(str)
end

function dvpassive!(xdot, x, p, t; 
                    mollify::Bool = true,
#                     useCurrentDensity::Bool=true,
                    useMonitor=false)
    # TODO: DONE 2023-02-23 09:04:59
    # decide wether to simplify by just using ρX and γXᵤ for channel X 
    # and calculate the derived vars here (γX and γXₘ) - although this brings 
    # more function calls here
    # OR: keep as is and use the parameters structure (@with_kw struct ...) to
    # calculate the derived vars (γX, γXₘ) from ρX and γXᵤ
    # 
    # if you want to play around with various values for the ρX you have to generate
    # a new parameters struct anyway (via reconstruct or from scratch using 
    # their c'tor) so probably better to stick with the latter approach
    # (currently used) - it would also save us from passing too many Units objects
    # around via 'p'
    
    # NOTE: 2023-02-23 09:06:05
    # for a typical passive Vm evolution you need γ𝓁ᵤ if at least 1 pS !
    
#     global t_vec,  Iinj_vec
#     global v_vec,  ∂v_vec
#     global I𝓁_vec, I𝓁ₘ_vec
#     global Iₑ_vec, Iₑₘ_vec
#     global Iₜ_vec, Iₜₘ_vec
#     global sampletimes
    
    C, γ𝓁, E𝓁, t₀, t₁, Iinj, uᵥ, uₜ, uᵢ = p;
    
    """Membrane voltage at this time point.
    
        This is either: 
        
        * the INITIAL CONDITION (i.e., the Eₘ in the parameters vector 'p') at
            the start of the differential equation solving process
            
        * the membrane voltage calculated by the previous iteration of the solver
    
    **NOTE:** the integrator (and solver) do not seem to cope well with Unitful 
    Quantities especially when comparing time variables in each step;
    
    So, best is to feed them Float64 values i.e. stripped from Units, but we do 
    bring those units back into the calculation of currents etc so that we 
    circumvent dimensional scaling business...
    """
    v = x[1]
    vv = v * uᵥ  
    
    """
    Unitful membrane voltage scaled to mV & stripped, 
    NOTE: THIS is what we feed to the differential eqns here.
    see NOTE: 2022-08-30 16:29:24
    """
    v_  = stripUnits(vv, u"mV")
    
    """Unitful time
    """
    tₜ = t * uₜ
    
#     uₐ = unit(Aₘ) # pass it through p, saves a function call
    
    # this is OK with Iinj as a Quantity
    """Unitful injected (electrode) current
    
    If `mollify`is `true` i_inj is created by a "mollfied" step - smooth 
    approximation to two Heaviside functions (OFF → ON then ON → OFF)
    Otherwise, it is just two Heaviside functions (OFF → ON then ON → OFF)
    
    """
    Iₑ = ifelse(mollify, somaticCurrentInjection(tₜ, t₀, t₁, Iinj), tₜ ≥ t₀ && tₜ < t₁ ? Iinj : zero(Iinj))
    
    """membrane 'leak' curent driven by E𝓁 - v given γ𝓁"""
    I𝓁  = γ𝓁  * (E𝓁 - vv) # A, see DIMENSIONAL ANALYSIS above; should be ZERO at initial condition when v == Eₘ !!!
    #I𝓁  = γ𝓁  * (vv -E𝓁) # WARNING -> MESSY
    """total current = membrane 'leak' current + injected current"""
    Iₜ  = I𝓁 + Iₑ       # A; should be ZERO at initial condition (v == Eₘ AND no current injection) 
    """rate of change in membrane potential - using currents
    NOTE: 2022-08-30 16:14:58
    using current densities or just currents in the calculation of ∂v, below,
    should yield the same result (with the same quantity dimensions)
    
    
    DON'T MIX THEM UP:
    in the first case divide current density by SPECIFIC mb capacitance
    IXₘ / Cₘ so we have A/m² / F/m² => A/F = V/s
    the second case is even more trivial: divide current by mb capacitance 
    IX/C
    
    Must scale to given voltage & time units, then strip to Float64"""
    ∂v = stripUnits(Iₜ/C, uᵥ/uₜ) # dimensionless

#     if useCurrentDensity
#         """injected current density: i_inj/Aₘ"""
#         Iₑₘ = i_inj/Aₘ # A/m²; injected current density ("electrode" current)
#         """membrane 'leak' curent density driven by E𝓁 - v given γ𝓁ₘ → ALWAYS present!"""
#         I𝓁ₘ = γ𝓁ₘ * (E𝓁 - vv) # A/m², see DIMENSIONAL ANALYSIS above; should be ZERO at initial condition when v == Eₘ !!!
#         """total current density = membrane 'leak' current density + injected current density"""
#         Iₜₘ  = I𝓁ₘ + Iₑₘ       # A/m²; should be ZERO at initial condition (v == Eₘ AND no current injection) 
#         """rate of change in membrane potential - using current DENSITIES
#         Must scale to given voltage & time units, then strip to Float64"""
#         ∂v = stripUnits(Iₜₘ/Cₘ, uᵥ/uₜ)
#     else
#         """injected current: i_inj"""
#         Iₑ  = i_inj # A; injected current ("electrode" current)
#         """membrane 'leak' curent driven by E𝓁 - v given γ𝓁"""
#         I𝓁  = γ𝓁  * (E𝓁 - vv) # A, see DIMENSIONAL ANALYSIS above; should be ZERO at initial condition when v == Eₘ !!!
#         """total current = membrane 'leak' current + injected current"""
#         Iₜ  = I𝓁 + Iₑ       # A; should be ZERO at initial condition (v == Eₘ AND no current injection) 
#         """rate of change in membrane potential - using currents
#         NOTE: 2022-08-30 16:14:58
#         using current densities or just currents in the calculation of ∂v, below,
#         should yield the same result (with the same quantity dimensions)
#         
#         
#         DON'T MIX THEM UP:
#         in the first case divide current density by SPECIFIC mb capacitance
#         IXₘ / Cₘ so we have A/m² / F/m² => A/F = V/s
#         the second case is even more trivial: divide current by mb capacitance 
#         IX/C
#         
#         Must scale to given voltage & time units, then strip to Float64"""
#         ∂v = stripUnits(Iₜ/C, uᵥ/uₜ) # dimensionless
#     end
    
    "store in xdot"
    xdot[1] = ∂v # dimensionless
    
#     # CAUTION HERE: somehow the UnitfulRecipes don't work when constructing a
#     # DataFrame with series of Vector{Quantity{Float64}}
#     # therefore we store them as stripped floats after scaling to correct units
#     # them re-attach the units in the runModel(...) caller AFTER we've collected
#     # them in a DataFrame
#     if t in sampletimes
#         push!(t_vec,    t)
#         push!(Iinj_vec, stripUnits(Iinj, uᵢ))
#         push!(∂v_vec,   ∂v)
#         push!(Iₑ_vec,   stripUnits(Iₑ,   uᵢ)) # current density!
#         push!(Iₑₘ_vec,  stripUnits(Iₑₘ,  uᵢ/uₐ)) # current density!
#         push!(I𝓁_vec,   stripUnits(I𝓁,   uᵢ))
#         push!(I𝓁ₘ_vec,  stripUnits(I𝓁ₘ,  uᵢ/uₐ))
#         push!(Iₜ_vec,   stripUnits(Iₜ,   uᵢ))
#         push!(Iₜₘ_vec,  stripUnits(Iₜₘ,  uᵢ/uₐ))
#         push!(v_vec,    x[1])
#     end
    
end

function makeParamatersType(name::Symbol, fields::AbstractDict{Symbol, Any})
    fielddefs = quote end
    ret = quote @with_kw struct $name <: AbstractParameters end
    
    ret = Expr(:struct, $name <: AbstractParameters)
end

"""Dynamic creation of new AbstractParameters subtype containing fields from 
two different AbstractParameters subtypes.

# Signatures

$(TYPEDSIGNATURES)
# Details

The function generates a new Struct-like DataType named `typename` and its 
custom unpacking macro entitled `@unpack_typename` in the namespace of the 
CTModels module.

The new DataType and the customized unpacking macro are then exported into the 
namespace of the Main module (so they are accessible directly from REPL).

The parameter structs passed to the function in the `paramStructs` argument must
either inherit from AbstractParameters (if theye are instances) or be a 
subtype of AbstractParameters (if they are types). Failing that, the function 
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

In other words, given p0, p1, p2, p3, p4 instances of DISTINCT `AbstractParameters`
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
@with_kw struct ParametersType1 <: AbstractParameters
    a::Int64 = 1
    b::Float64 = 2.3
end

# create an instance of ParametersType1 with default values
params1 = ParametersType1() 

# create another custom type:
@with_kw struct ParametersType2 <: AbstractParameters
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
    if ! isa(p0, AbstractParameters)
        @error "Expecting an instance of an AbstractParameters subtype"
    end
    f0 = stringify(p0)
    
    for p in paramStructs[2:end]
        if !isa(p, AbstractParameters)
            @error "Expecting an instance of an AbstractParameters subtype"
        end
#         println(typeof(p))
        pf = stringify(p)
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

"""Naᵥ 'h' gating particle (inactivation) - Migliore et al 1999"""
function Naᵥ_h_gate(v::Union{Int64, Float64}; useHH::Bool=false)
    if useHH
        α  = alphaₕHH(v)
        β  = betaₕHH(v)
    else
        α  = alphaₕMJ(v)
        β  = betaₕMJ(v)
    end
    # τ = 1 / (α + β)
    #  x∞ = α * τ
    τ  = max(0.5 / (α + β), 0.5) # time constant of gating variable h
    x∞ = 1/(1 + exp((v+50)/4))
    
    # ∂h = αₕ * (1 - h) - βₕ * h
    # alternatively, for temperature dependency: 
    # τₕ = 1 / (qNa10 * (αₕ + βₕ))
    # τₕ = 1 / (αₕ + βₕ) # time constant of gating variable h
    # h∞ = αₕ * τₕ
    
    return x∞, α, β, τ
end

"""Naᵥ 'i' gating particle ('slow' inactivation) - Migliore et al 1999"""
function Naᵥ_i_gate(v::Union{Int64, Float64}, bᵢ::Union{Int64, Float64}=1; useHH::Bool=false)
    α = alphaᵢMJ(v)
    β = betaᵢMJ(v)
    τ = max(3e4 * β / (1 + α), 10)
    eterm = exp(v + 58)/2.0
    x∞ = (1 + bᵢ * eterm) / (1 + eterm)
    return x∞, α, β, τ
end

"""Naᵥ 'i' gating particle ('slow' inactivation) - Migliore et al 1999"""
function Naᵥ_i_gate(v::Float64, bᵢ::Int64=1; useHH::Bool=false)
    α = alphaᵢMJ(v)
    β = betaᵢMJ(v)
    τ = max(3e4 * β / (1 + α), 10)
    eterm = exp(v + 58)/2.0
    x∞ = (1 + bᵢ * eterm) / (1 + eterm)
    return x∞, α, β, τ
end




"""Nernst (reversal) potential for ion X given [X]ᵢ, [X]ₒ, temperature t and ion's valence zₓ
$(TYPEDSIGNATURES)
"""
Nernst(Xᵢ::Concentration{T}, Xₒ::Concentration{U}, zₓ::Int64, t::Unitful.Temperature{V}) where {T<:Real, U<:Real, V<:Real} = Unitful.R * uconvert(u"K", t) * 
                                                                                                                                    log(Xₒ/Xᵢ) / (zₓ * 𝑭)

"""Creates an OrderedDict from a parameters struct"""
params2dict(x::AbstractParameters) = OrderedDict(sort(collect(type2dict(x)), by=(y)->y[1]))

function R!(rates, xc, xd, p, t)
end

function runPDMP(params::ModelParameters, tspan::Union{NTuple{2,Union{Float64, Unitful.Time{Float64}}},Nothing}=nothing; 
                 mollify::Bool=true,
                 useRV::Bool = true,
                 useCurrentDensity::Bool=true, # this, just to verify dv! works
                 asDataFrame::Bool=true, 
                 useMonitor::Bool=false,
                 kwargs...)

    @unpack_ModelParameters params
    
    #BEGIN Calculated (derived) parameters
    
    """Surface area"""
    Aₘ::Unitful.Area{Float64} = π * D^2 

    """Membrane electrical capacitance"""
    C::Unitful.Capacitance{Float64} = Cₘ * Aₘ

    """'Leak' channel electrical conductance density"""
    γ𝓁ₘ::ElectricalConductanceDensity{Float64} = γ𝓁ᵤ * 𝒏𝓁
    
    """'Leak' channel electrical conductance"""
    γ𝓁::Unitful.ElectricalConductance{Float64} = γ𝓁ₘ * Aₘ

    """Maximal Naᵥ conductance density;
    i.e. when all Naᵥ are fully open"""
    γNaₘ::ElectricalConductanceDensity{Float64} = γNaᵤ * 𝒏Na
    
    """Maximal Naᵥ conductance"""
    γNa::Unitful.ElectricalConductance{Float64} = γNaₘ * Aₘ

    """Maximal Kᵥ conductance density;
    i.e. when all Kᵥ are fully open"""
    γKₘ::ElectricalConductanceDensity{Float64} = γKᵤ * 𝒏K
    
    """Maximal Kᵥ conductance"""
    γK::Unitful.ElectricalConductance{Float64} = γKₘ * Aₘ
    
    #END Calculated (derived) parameters

    uₗ = unit(D)
    uₐ = unit(Aₘ)
    uᵥ = unit(E𝓁)
    uᵪ = unit(C)
    uᵧ = unit(γ𝓁)
    uₙ = unit(𝒏𝓁) # 1/uₐ
    uᵢ = unit(Iinj)
    
    uₜ₀ = unit(t₀) # save for later
    uₜ = unit(1u"ms")
    
    # see NOTE: 2022-09-01 12:33:47
    t₀ = uconvert(uₜ, t₀)
    t₁ = uconvert(uₜ, t₁)

    if tspan == nothing
        tspan = timespan # provided by ModelParameters params
    end

    # solver doesn't do well with time Quantities ?
    tspan = map((x) -> stripUnits(x, uₜ), tspan) 

    xc₀ = [stripUnits(Eₘ, uᵥ), 0., 1., 0.] 
    
    problem = PDMP.PDMPProblem( F!, R!, nu, xc₀, xd₀, p, tspan)
    
    sol = solve(problem, CHV(:lsoda); kwargs...)
    
end

"""Returns a string representation of a subtype of AbstractParameters, or of 
an instance of AbstractParameters subtype.
"""
function stringify(p::Union{AbstractParameters, DataType})
    ff = if p isa AbstractParameters 
        # BUG:2023-02-15 10:10:29 FIXME/TODO
        # currently I cannot get the parser to properly resolve the typespec string
        # so Unitful.Quantity fields will revert to Any in the merged struct
        # calls stringify in utils.jl -> only for units! (that definition should be moved to units.jl?)
        return collect(map((x) -> isa(getproperty(p,x), Quantity) ? String(x)*" = "*stringify(getproperty(p, x)) : String(x)*" = "*stringify(getproperty(p,x)), fieldnames(typeof(p))))
#         return collect(map((x) -> isa(getproperty(p,x), Quantity) ? String(x)*stringifyQuantityType(getproperty(p,x))*" = "*stringify(getproperty(p, x)) : String(x)*" = "*stringify(getproperty(p,x)), fieldnames(typeof(p))))
#         return collect(map((x) -> isa(getproperty(p,x), Quantity) ? String(x)*"::Unitful.Quantity = "*stringify(getproperty(p, x)) : String(x)*" = "*stringify(getproperty(p,x)), fieldnames(typeof(p))))
    elseif supertype (p) == AbstractParameters # p is a Type so we instantiate it here
        pp = p()
        return collect(map((x) -> String(x)*"::Unitful.Quantity = "*stringify(getproperty(pp, x)), fieldnames(p))) # collect default values
    else
        @error "Expecting an AbstractParameters subtype or an instance of it"
        
    end
end

"""In progress - tying to fix BUG:2023-02-15 10:10:29 """
 function stringify2(p::Union{AbstractParameters, DataType})
    ff = if p isa AbstractParameters 
        # BUG:2023-02-15 10:10:29 FIXME/TODO
        # currently I cannot get the parser to properly resolve the typespec string
        # so Unitful.Quantity fields will revert to Any in the merged struct
        # calls stringify in utils.jl -> only for units! (that definition should be moved to units.jl?)
        return collect(map((x) -> isa(getproperty(p,x), Quantity) ? String(x)*" = "*stringify(getproperty(p, x)) : String(x)*" = "*stringify(getproperty(p,x)), fieldnames(typeof(p))))
#         return collect(map((x) -> isa(getproperty(p,x), Quantity) ? String(x)*stringifyQuantityType(getproperty(p,x))*" = "*stringify(getproperty(p, x)) : String(x)*" = "*stringify(getproperty(p,x)), fieldnames(typeof(p))))
#         return collect(map((x) -> isa(getproperty(p,x), Quantity) ? String(x)*"::Unitful.Quantity = "*stringify(getproperty(p, x)) : String(x)*" = "*stringify(getproperty(p,x)), fieldnames(typeof(p))))
    elseif supertype(p) == AbstractParameters # p is a Type so we instantiate it here
        pp = p()
        return collect(map((x) -> String(x)*"::Unitful.Quantity = "*stringify(getproperty(pp, x)), fieldnames(p))) # collect default values
    else
        @error "Expecting an AbstractParameters subtype or an instance of it"
        
    end
end

@inline function test_inline(a,b,c)
end

function test_where (a::T) where T<:AbstractParameters
end

@inline function test_where (a::T) where T<:AbstractParameters
end

# """A generalized unpacking of structs.
# 
# $(TYPEDSIGNATURES)
# 
# Useful to unpack the fields of any struct generated with the `@with_kw` macro in
# the `Parameters.jl` , instead of the less flexible calls such as:
# 
# `@unpack_TypeName x` or
# 
# `Parameters.@unpack a,b = x`
# 
# where `x::TypeName` is an instance of a `@with_kw`-generated struct called TypeName
# or an instance of a Dict.
# 
# **NOTE:** The `@unpack_TypeName` macro is generated when defining a struct using 
# the `@with_kw` macro, and its suffix after the underscore is identical to the 
# type name of the generated struct; the `Parameters.@unpack` macro is reexported
# from the `UnPack.jl` package.
# 
# **NOTE:** The macro is named `@unpackObj` to avoid overriding the `@unpack`
# macro exported by `Parameters.jl`
# 
# """
# macro unpackObj(p)
#     local s = eval(getproperty(__module__, p))
#     local fields = fieldnames(typeof(s));
#     local sym = Symbol(__module__, ".", p)
#     
#     esc(Expr(:block, [:($f = $s.$f) for f in fields]...));
#         
# end
# 
# """A version of the `@unpackObj` macro restricted to AbstractParameters
# $(TYPEDSIGNATURES)
# Only works at module level
# """
# macro unpackParams(p)
#     local s = eval(getproperty(__module__, p))
#     @assert supertype(typeof(s)) == AbstractParameters
#     local fields = fieldnames(typeof(s));
#     local sym = Symbol(__module__, ".", p)
#     
#     esc(Expr(:block, [:($f = $s.$f) for f in fields]...));
#         
# end
    
    """A generic macro for unpacking of structs - BROKEN, DON'T USE !!!

$(TYPEDSIGNATURES)

Useful to unpack the fields of any struct generated with the `@with_kw` macro 
(provided by the `Parameters.jl` package), regardless of the type name (symbol)
of that struct.

Useful to unpack structs created dynamically (w.g. with mergeParameters) instead
of the more static (but more specific) calls such as:

`@unpack_TypeName x` or `Parameters.@unpack a,b = x` , where `x::TypeName` is 
an instance of a `@with_kw`-generated struct called TypeName, or an instance of 
a Dict.

**NOTE:** The `@unpack_TypeName` macro is generated when defining a struct using 
the `@with_kw` macro, and its suffix after the underscore is identical to the 
type name of the generated struct; the `Parameters.@unpack` macro is reexported
from the `UnPack.jl` package.

**NOTE:** The macro is named `@unpackParameters` to avoid overriding the 
`@unpack` macro exported by `Parameters.jl`

**ATTENTION**

this macro MUST be called as

```julia
@unpackParameters(p)
```

Where 'p' is an instance of AbstractParameters

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
    

 macro unpackParameters2(pstruct)
    # field symbols -> this, eval'ed at REPL, resolves to the list of symbols in :pstruct
    # the symbols and obkjects bound to them become available in the caller
    # namespace
    aa = Vector()
    bb = Vector()
    cc = Vector()
    fields = fieldnames(typeof(pstruct))
    push!(aa, fields...)
    push!(bb, [getproperty(pstruct, f) for f in fields]...)
    push!(cc, [:($f=$v) for (f,v) in zip(aa,bb)]...)
            
    fexp = Expr(:call, fieldnames, Expr(:call, typeof, :($pstruct)))
#     fexp = Expr(:call, fieldnames, :($pstruct))
    ret = quote
        esc($cc)
#         local val = $pstruct
#         local fsyms = $fexp
#         eval(Expr(:block, [:($f=$val.$f) for f in fsyms]...))
#         esc(Expr(:block, [:($f=$val.$f) for f in fsyms]...))
    end
    
#     return cc

#         return eval(esc(ret))
#     return ret
end

"""Gating "variable" functions
All take one parameter: `v` which is membrane voltage - a scalar (mV).

These functions are, essentially, scaled Boltzmann-like functions of the form:

y = α/(1+exp((v ± V½)/κ)) 

where: 

`α`  : scale factor

`v`  : the membrane voltage

`V½` : the half-activation voltage

`κ`  : a "slope" (or "shape") factor

`y`  : adimensional numeric vector

The denominators in the exponentials are in ms (time constants)!
Results are dimensionless
NOTE: 2023-02-11 16:02:58 
The unitful versions are experimental - do NOT use!
"""

#BEGIN NOTE: 2022-09-01 23:25:36 


#END NOTE: 2022-09-01 23:25:36


@with_kw struct ModelParameters <: AbstractParameters
    #BEGIN Morphology parameters
    """Diameter of a spherical cell model."""
    D::Unitful.Length{Float64} = 30.0u"μm"
    # D::Unitful.Length{Union{Int64,Float64}} = 30.0u"μm"
    #D::Unitful.Length{T} where T = 30.0u"μm"
    #END Morphology parameters
    
    #BEGIN -> Passive membrane parameters
    """*Specific* membrane capacitance."""
    Cₘ::SpecificMembraneCapacitance{Float64} = 1.54e-2u"pF/μm^2" # 1.54 μF/cm² = 0.0154 pF/μm² = 0.0154 F/m²
#     Cₘ::CapacitanceDensity{Float64} = 1.54e-2u"pF/μm^2" # 1.54 μF/cm² = 0.0154 pF/μm² = 0.0154 F/m²
    #Cₘ::CapacitanceDensity{T} where T = 1.54e-2u"pF/μm^2" # 1.54 μF/cm² = 0.0154 pF/μm² = 0.0154 F/m²
    """Initial (resting) membrane potential
    """
    Eₘ::Unitful.Voltage{Float64} = -70.0u"mV"
    #Eₘ::Unitful.Voltage{T} where T = -70.0u"mV"
    #END -> Passive membrane parameters
    
    #BEGIN Leak parameters
    """Generic 'leak' channel density
    """
    𝒏𝓁::SurfaceDensity{Float64} = 1.0u"μm^(-2)" # 1/μm²
    #𝒏𝓁::SurfaceDensity{T} where T = 1u"μm^(-2)" # 1/μm²
    
    """Unitary (a.k.a single channel) conductance - generic 'leak' channels (pS)
    """
    γ𝓁ᵤ::Unitful.ElectricalConductance{Float64} = 0.1u"pS"
    #γ𝓁ᵤ::Unitful.ElectricalConductance{T} where T = 0.1u"pS"

    """Resting membrane potential"""
    E𝓁::Unitful.Voltage{Float64} = -70.0u"mV"
    #E𝓁::Unitful.Voltage{T} where T = -70.0u"mV"
    
    #END LeakParameters
    
    #BEGIN Na Ion Parameters
    zNa::Int64 = 1
    Naᵢ::Concentration{Float64} = 20.0u"mM" # Na₂-Phosphocreatine 10 mM, Na-GTP 0.3 mM
    Naₒ::Concentration{Float64} = 119.0u"mM"
    #Naᵢ::Concentration{T} where T = 20.0u"mM" # Na₂-Phosphocreatine 10 mM, Na-GTP 0.3 mM
    #Naₒ::Concentration{T} where T = 119.0u"mM"
    #END Na Ion Parameters
    
    #BEGIN NaV parameters
    # NOTE: 2023-02-12 23:56:41
    # for a D of 30 μm, 18 Na⁺ channels / μm² with unitary slope conductance of
    # γ 17 pS (see e.g., Alzheimer et al, 1993, Baker & Bostock, 1998, Fernandes et al, 2001)
    # this gives a total maximum (peak) Na⁺ conductance of 320 pS/μm² (Migliore et al, 1999)
    # in neurons; (but see ~30 pS in neuroblastoma cells, Aldrich & Stevens, 1987)
    """Naᵥ channel density"""
    𝒏Na::SurfaceDensity{Float64} = 18.0u"μm^(-2)"

    """Unitary (a.k.a single channel) conductance, Naᵥ"""
    γNaᵤ::Unitful.ElectricalConductance{Float64} = 17.0u"pS"

    """Na⁺ equilibrium potential."""
    ENa::Unitful.Voltage{Float64} = 50.0u"mV" 
    
    bᵢ::Float64 = 0.8 # See Migliore et al 1999; 0.5 in apical dendrites, 0.8 in soma, 1 elsewhere
    #bᵢ::T where T = 0.8 # See Migliore et al 1999; 0.5 in apical dendrites, 0.8 in soma, 1 elsewhere
    
    #END NaV parameters
    
    #BEGIN K Ion parameters
    zK::Int64 = 1
    Kᵢ::Concentration{Float64} = 134.2u"mM"
    Kₒ::Concentration{Float64} = 2.5u"mM"
    #END K Ion parameters

    #BEGIN KV parameters 
    """Kᵥ channel density - delayed rectifier (which one ?!?)
    """
    𝒏KDR::SurfaceDensity{Float64} = 10.0u"μm^(-2)" # to vary - references?
    #𝒏KDR::SurfaceDensity{T} where T = 10u"μm^(-2)" # to vary - references?
    
    """Unitary (a.k.a single channel) conductance, DR-type Kᵥ (delayed rectifier)"""
    γKDRᵤ::Unitful.ElectricalConductance{Float64} = 10.0u"pS" # Migliore et al, 1999
    #γKDRᵤ::Unitful.ElectricalConductance{T} where T = 10.0u"pS" # Migliore et al, 1999
    
    
    """Unitary conductance for A-type Kᵥ 
    Default value here is for soma.
    For dendrites, according to Migliore et al 1999 this would be:
    
    • 48 pS * (1+d/100) for d ≤ 100 μm from soma
    • 0  pS otherwise
    
    i.e., they are prominent at soma then increase linearly with distance from 
    soma along the proximal dendrites up to 100 μm where it doubles; finally, it
    vanishes beyond 100 μm from soma
    
    -- not that's funny: is the unitary conductance that changes or the number 
    of channels ?
    
    """
    𝒏KA::SurfaceDensity{Float64} = 64.0u"μm^(-2)"
    #𝒏KA::SurfaceDensity{T} where T = 64u"μm^(-2)"
    γKAᵤ::Unitful.ElectricalConductance{Float64} = 7.5u"pS" # at 0 μm from soma -- Migliore et al 1999
    #γKAᵤ::Unitful.ElectricalConductance{T} where T = 7.5u"pS" # at 0 μm from soma -- Migliore et al 1999

    """K⁺ equilibium potential."""
    EK::Unitful.Voltage{Float64} = -90.0u"mV" 
    #EK::Unitful.Voltage{T} where T = -90.0u"mV" 
    #END KV parameters
    
    #BEGIN Ca Ion parameters
    zCa::Int64 = 2
    Caᵢ::Concentration{Float64} = 100.0u"μM"
    Caₒ::Concentration{Float64} = 2.5u"mM"
    #Caᵢ::Concentration{T} where T = 100.0u"μM"
    #Caₒ::Concentration{T} where T = 2.5u"mM"
    #END Ca Ion parameters
    
    #BEGIN CaV parameters
    """T-type channels: CaV3.x"""
    𝒏CaT::SurfaceDensity{Float64} = 5.0u"μm^(-2)"
    γCaTᵤ::Unitful.ElectricalConductance{Float64} = 10.0u"pS" # Table 1 in Magee & Johnston 1995 Characterization...
    """R-type channels: CaV2.3"""
    𝒏CaR::SurfaceDensity{Float64} = 3.0u"μm^(-2)"
    γCaRᵤ::Unitful.ElectricalConductance{Float64} = 17.0u"pS" # Table 1 in Magee & Johnston 1995 Characterization...
    """L-type channels: CaV1.x"""
    𝒏CaL::SurfaceDensity{Float64} = 3.0u"μm^(-2)"
    γCaLᵤ::Unitful.ElectricalConductance{Float64} = 27.0u"pS" # Table 1 in Magee & Johnston 1995 Characterization...
    #END CaV parameters
    
    #BEGIN Experiment parameters
    """Injected current. The sign indicates if injected current is inward 
    (hyperpolarizing, < 0) or 
    outward (depolarizing, > 0).
    """
    Iinj::Unitful.Current{Float64} = 1.0e3u"pA"

    """Time of current injection start."""
    t₀::Unitful.Time{Float64} = 50.0u"ms"
    
    """Time of current injection end."""
    t₁::Unitful.Time{Float64} = 250.0u"ms"
    @assert t₁ > t₀
    
    """Temperature in °C.
    To convert to °K use:
    `uconvert(u"K", t)` where t is , say, 35u"°C"
    
    CAUTION: this returns a rational number, which Julia should handle.
    
    If in doubts, use
    
    `float(uconvert(u"K", t))` --> floating point number (in °K),e.g.:
    
    ```julia
    float(uconvert(u"K", 35u"°C"))
    
    308.15 K
    ```
    """
    temperature::Unitful.Temperature{Float64} = 35.0u"°C"
    #END Experiment parameters
    
    #BEGIN Simulation parameters
    timespan::NTuple{2, Unitful.Time{Float64}} = (0.0u"ms", 500.0u"ms")
    #END Simulation parameters
    
end



@with_kw mutable struct NaVParameters <: AbstractParameters
    """Naᵥ channel density"""
    𝒏Na::SurfaceDensity{Int64} = 18u"μm^(-2)"

    """Unitary (a.k.a single channel) conductance, Naᵥ"""
    γNaᵤ::Unitful.ElectricalConductance{Float64} = 17.0u"pS"

    bᵢ::Union{Int64,Float64} = 0.8 # See Migliore et al 1999; 0.5 in apical dendrites, 0.8 in soma, 1 elsewhere
    
end

mutable struct TestMe
end

struct TestMe2 
end


    


a = b
