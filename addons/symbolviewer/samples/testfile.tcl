#! /bin/env tclsh
set global1 "global1"

proc sum {x y} {
    set sum [expr {$x + $y}]
    return sum
}

proc printParams params {
    foreach arg $args {
        puts $arg
    }
}
