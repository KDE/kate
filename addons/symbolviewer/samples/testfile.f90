module pi

   real, parameter :: pi = 3.14

contains
   subroutine print_pi()
      print *, "Pi = ", pi
   end subroutine print_pi

end module pi

integer function foo1(param1) result(res)
  res = param1 + 1
end function foo1

subroutine sub1(param1, param2)
  print *, "sub1(", param1, ", ", param2, ")"
end subroutine sub1

program program1
  ! Comment line
  use pi
  print_pi()
  print *, "foo1(1) => ", foo1(1)
  sub1(1, 2)
end program program1
