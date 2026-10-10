begin
    comment library 1;
    real
        a,
        x;
    x := 0.8;
    a := (if x >= 0 & x <= 1 then x * x - 3 * x + 4 else
        0);
       outreal(a);
end
