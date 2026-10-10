begin

    comment library 1;
    integer
        i;
    real
        x;
    procedure ass(x, i);
        real
            x;
        integer
            i;
        if i < 0 then
            x := i + 3
        else
            i := 2.0 * x;

    ass(x, - 1);
    i := 2;
    ass(x, i);
    ass(i, i);
    ass(i, x);
    outreal(1, i);
    outreal(1, x)
end
