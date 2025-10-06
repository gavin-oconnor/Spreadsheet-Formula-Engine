Tokens
Cell Reference (A1)
Numeric literals (int and float)
String literals ("hello, world")
Separators (comma)
operators (+-/\*^:)
function literal (SUM, should match anything in form WORD(expr,expr,expr))
r_paren (
l_paren )

Limitations (self-imposing for now to make this easier)

## Pls Fix's

Blank cells (as scalars) should be some kind of null value that gets handled differently by different operations
Bounds checking needed in evaluating cell reference as scalar
FUNCTION TYPE COERCION (EX: LEN(5.0) = 3)
