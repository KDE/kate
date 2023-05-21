
"""Some multi-line comment documenting FooParams
• foo
• bar
• baz
"""
abstract type FooParams end

"""Do NOT try this at home!"""
@with_kw struct Parameters <: FooParams
    
end



"""Do NOT try this at home!"""
@with_kw mutable struct MutableParameters <: FooParams
    
end

mutable struct Concentration{T} where T
    x::T
end

struct TestMe2 
end



@inline Foo_αₕ(v::Union{Int64, Float64}) = 0.07  * exp(-(v + 65)/20) 
Fooβₕ(v::Union{Int64, Float64}) = 0.07  * exp(-(v + 65)/20) 

  Base.display(x::FooParams) = disp(x)


"""Adds a method to a function defined in the Base module"""
@inline function Base.MethodOfBaseFoo (v::Union{Int64, Float64}; useMagic::Bool=false) 
end

"""Adds a method to a Base function.
Returns pairs of key/values from a FooParams type
# Signatures

$(TYPEDSIGNATURES)
"""
Base.pairs(x::FooParams; strip::Bool=false) = ifelse(strip, 
                                                           map((f) -> f => stripUnits(getproperty(x, f)), fieldnames(typeof(x))),
                                                           map((f) -> f => getproperty(x, f), fieldnames(typeof(x))),
                                                          )


baz(v, V½, 𝒌) = 1/(1+exp((V½-v)/𝒌))

function useless_
end

function disp(x::FooParams)
end

"""
$(TYPEDSIGNATURES)
"""
function bar!(x::Symbol,y::AbstractDict{Symbol, Any}, z...; 
             flag::Bool = true,
             flag2=false)
    
end


"""baz
$(TYPEDSIGNATURES)
"""
bar(Xᵢ::Concentration{T}, Xₒ::Concentration{U}, zₓ::Int64, t::Unitful.Temperature{V}) where {T<:Real, U<:Real, V<:Real} = Unitful.R * uconvert(u"K", t) * 
                                                                                                                                    log(Xₒ/Xᵢ) / (zₓ * 𝑭)

"""Creates an OrderedDict from a parameters struct"""
params2dict(x::FooParams) = OrderedDict(sort(collect(type2dict(x)), by=(y)->y[1]))

function baz2!(rates, xc, xd, p, t)
end

function broken_params_line(param1::FooParams, param2::Union{NTuple{2,Union{Float64, Unitful.Time{Float64}}},Nothing}=nothing; 
                 flag1::Bool=true,
                 flag2::Bool = true,
                 kwargs...)

    
end


@inline function inline_function(a,b,c)
end

@noinline function noinline_function(a,b,c)
end

@noinline shortdef(x) = ...


function test_where (a::T) where T<:FooParams
end

@inline function test_where2 (a::T) where T<:FooParams
end

# anonymous function 
f() do
    @noinline
    ...
end 

macro sayhello()
    return :( println("Hello, world!") )
end

macro twostep(arg)
    println("I execute at parse time. The argument is: ", arg)
    return :(println("I execute at runtime. The argument is: ", $arg))
end



    


a = b
