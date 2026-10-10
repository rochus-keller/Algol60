begin

    comment library 1;

    integer
        i;
    real
        x;
    i := 1;
    for x := .1 step x until 1.0#6 do
        i := i + 1;
    outreal(1, i)
end
