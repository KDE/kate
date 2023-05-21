#!/bin/env python3
import typing

def foo1():
    """function w/o params"""
    pass

if toto == True:
    def foo2():
        pass
else:
    def foo2():
        pass

class β:
    def α () :
        pass
    pass
    
class class1:
    pass

def foo3(param):
    pass

def foo4(param1, param2="default") -> float:
    pass
    
def foo5 (a, /, param1 : int , param2 : typing.Optional[typing.Union[typing.Sequence[typing.Any], typing.Mapping]]=None)->typing:Mapping[str, typing.Sequence[int]]:
    """function with params annotations and return annotations"""
    pass

def foo6 (a, /, param1:int, 
          param2:typing.Optional[typing.Union[typing.Sequence[typing.Any], typing.Mapping]]=None) -> typing:Mapping[str, typing.Sequence[int]]:
    """function with params annotations and return annotations
        params on multiple lines WITHOUT line continuation character
    """
    pass

class class2 (class1, β ):
    pass

class class³(class1, 
             toto=1):
    pass

class class4():
    def __init__(self):
        pass
        
    def method_with_annotated_return(self) -> typing.Sequence:
        pass
        
    def foo1_in_class4(self, /, param1:float,
                       param2:int) -> list:
        pass
        
    def foo2_in_class4(self, *args, **kwargs) -> bool:
        pass
        
    def τ(self, α:float):
        pass
