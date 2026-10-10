begin

    comment library 1;

    real procedure g(x);
        real
            x;
        g := x + 1;

    real procedure a(h);
        real procedure
            h;

        comment   (h1): real h1;

        begin
        real procedure loc(x);
            real
                x;
            loc := h(g(x));

        a := loc(0);
        end a;

    real procedure b(f, h);
        real procedure
            f;

        comment   (f1): real f1;

        real procedure
            h;

        comment   (h1): real procedure h1;

        b := f(h(f) + a(g) + g(1));

    outreal(1, b(g, a))
end
