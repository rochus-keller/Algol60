begin

    comment library 1;

    integer
        x,
        n,
        k;
    real procedure sigma(i, k, l, t);
        value
            l;
        integer
            i,
            k,
            l;
        real
            t;
        begin
        real
            x;
        x := 0;
        for i := k step 1 until l do
            x := t + x;
        sigma := x
        end sigma;

    x := sigma(n, 1, 4, sigma(k, 1, n, 2) - 1);

    comment   working in august 1960 at amsterdam;

    outreal(1, x)
end
