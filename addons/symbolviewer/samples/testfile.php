<?php
namespace namespace1;

define('CONSTANT_1', 1);
define("CONSTANT_2", 2);

class class1 {
    var $attribute1 = 0;
    public $public_attribute2 = 0;
    public static $public_static_attribute3 = 0;

    const CLASS_CONSTANT = 0;

    public function public_foo1_in_class1() { }
    public static function public_static_foo2_in_class1() { }
    protected function protected_foo3_in_class1() { }
    private function private_foo4_in_class1() { }
private function private_foo5_in_class1() { }
}

class classExtendClass1 extends class1 {
    function foo1_in_classExtendClass1() { }
}

trait trait1 {
    function foo1_in_trait1() { }
}

class classImplementInterface implements interface1 {
}

function foo1($param1, $param2=null) {
}

abstract class classAbstract {
}

final class classFinal {
}
