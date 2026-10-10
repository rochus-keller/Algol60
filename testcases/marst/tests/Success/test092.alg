begin
comment library 1;
    integer
        n;
    integer procedure p(q, r);
        integer procedure
            r,
            q;

        comment   (r):integer procedure r;

        begin
        integer
            b;
        b := 3 * q(r);
        p := b
        end;

    integer procedure a(f);
        integer procedure
            f;
        a := 2 * f;

    integer procedure b;
        begin
        n := n - 1;
        b :=
            if n = 0 then
                1
            else
                p(a, b) + 1
        end;

    n := 2;
    n := p(a, b);
    outreal(1, n)
end
