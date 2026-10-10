begin

    comment library 1;

    integer
        a;
    integer procedure f(x, y);
        value
            x,
            y;
        integer
            x,
            y;
        a := f := x + 1;

    integer procedure g(x);
        integer
            x;
        x := g := a + 2;

    a := 0;
    outreal(1, a + f(a, g(a)) / g(a))
end
