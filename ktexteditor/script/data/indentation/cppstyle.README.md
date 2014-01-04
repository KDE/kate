C++/boost Style Indenter
========================

This indenter (initially) was designed to help code typing in a boost::mpl style
(i.e. w/ leading comma in formatted parameters list). One may read rationale of such
approach in the "C++ Template Metaprogramming: Concepts, Tools, and Techniques from Boost and Beyond"
by David Abrahams and Aleksey Gurtovoy. It is really easy to miss a comma when "calling" templates and
it would leads to really complicated compile errors. This technique helps to visually control syntax
in a stricter way.

Example:

    typedef typename boost::mpl::eval_if<
        boost::is_same<iter, boost::mpl::end<types_map>::type>
      , boost::mpl::int_<-1>
      , boost::mpl::distance<boost::mpl::begin<types_map>::type, iter>
      >::type type;

In practice I've noticed that this style can be used to format long function calls or even
`for` statements. Actually everything that can be split into several lines could be formatted that way.
And yes, it is convenient to have a delimiter (comma, semicolon, whatever) as leading character to
make it visually noticeable.

    // Inheritance list formatting
    struct sample
      : public base_1
      , public base_2
      , ...
      , public base_N
    {
        // Parameter list formatting
        void foo(
            const std::string& param_1                      ///< A string parameter
          , double param_2                                  ///< A double parameter
          , ...
          , const some_type& param_N                        ///< An user defined type parameter
          )
        {
            //
            for (
                auto first = std::begin(param_1)
              , last = std::end(param_1)
              ; it != last
              ; ++it
              )
            {
                auto val = check_some_condition()
                  ? get_then_value()
                  : get_else_value()
                  ;
            }
        }
    };

It looks unusual for awhile :) but later it become "normal" and easily to read and edit :)
Really! When you want to add one more parameter to a function declaration or change order it will
takes less typing comparing to "traditional" way :) (especially if you have some help from editor,
like moving current line or selected block up/down by hotkey or having indenter like this :)

Features
--------

* support for boost-like formatting style everywhere is possible
* align inline comments to 60th position after typing `'//'`
* turn `'///'` into `'/// '` or `'///< '` depending on comment placement
* take care of inline comments when `ENTER` pressed on a commented line (before, middle or after comment start)
* turn `'/*'` or `'/**'` into multiline (doxygen style) comment
* auto continue multiline comment on `ENTER`
* indent preprocessor directives according nesting level of `#if`/`#endif`
* take care about backslashes alignment in preprocessor macro definitions
* append a space after comma or some keywords (like `if`,  `for`,  `while`,  etc...) or `operator<<`
* align access modifiers


TODO
----

Initially it was designed as typing helper and nowadays it lacks of align functionality (it's capable to
align only few basic constructs). But I have plans to add some logic here to better align source code keeping
C++/boost style in mind.
