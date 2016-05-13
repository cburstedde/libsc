
   dnl  euoahtns aoeu thnseuoa htnseoa  test "$ABC" = "no"
dnl     euoahtns aoeu thnseuoa htnseoa  test "$ABC" = "no"

dnl We don't need the quotes around literals
test "$ABC" = "no"                                  # should hit 0
test "$ABC" = "yes"                                 # should hit 1
test "x$ABC" = "xno"                                # should hit 2
test "x$ABC" = "xyes"                               # should hit 3

dnl We need to prefix the $ABC with x
test "$ABC" = no                                    # should hit 4
test "$ABC" = yes                                   # should hit 5

dnl These are ok
test "x$ABC" = xno
test "x$ABC" = xyes

dnl It is not safe to use test with -o or -a; use test 1 && test 2 instead
test 1 -a 2                                         # should hit 6
test 1 -o 2                                         # should hit 7

dnl It is not safe to use ! test; use test ! instead
if ! test -d 3 ; then echo ABC ; fi                 # should hit 8
