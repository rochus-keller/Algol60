COMMENT copied from https://rosettacode.org/wiki/Man_or_boy_test. 
Result is supposed to be −67. 
See also https://en.wikipedia.org/wiki/Man_or_boy_test;

begin
  real procedure A (k, x1, x2, x3, x4, x5);
  value k; integer k;
  real x1, x2, x3, x4, x5;
  begin
    real procedure B;
    begin k:= k - 1;
          B:= A := A (k, B, x1, x2, x3, x4)
    end;
    if k <= 0 then A:= x4 + x5 else B
  end;
  outreal (A (10, 1, -1, -1, 1, 0))
end
