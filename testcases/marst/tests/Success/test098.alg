begin

    comment library 1;

    real procedure r(a);
        real
            a;
        begin
        integer procedure p(b, c);
            integer
                b,
                c;
            p := b + c;

        r := p(a + 1, a)
        end r;

    real
        x;
    begin
        real
            y;
        y := 4;
        x := r(r(y + 1))
    end;
    outreal(1, x)
end
