begin

    comment library 1;

    procedure p(a, b, c, d);
        value
            b,
            c;
        real
            a,
            b;
        array
            c,
            d;
        begin
        a := a + 1;
        s := s + a + b + c[1, 2] + d[1, 2];
        d[1, 2] := 2
        end;

    integer array
        a[16384 : 16385, - 16383 : - 16382];
    integer
        s;
    array
        b[1 : 2, 0 : 2];
    a[16384, - 16383] := 3.0;
    b[1, 2] := a[16384, - 16383];
    begin
        integer array
            g[1 : b[1, 2], 0 : 2];
        s := 0;
        g[1, 0] := g[2, 0] := g[3, 0] := g[1, 1] := g[2, 1]
            := g[3, 1] := g[1, 2] := g[2, 2] := g[3, 2]
            := 1;
        p(b[1, 2], b[1, 2], g, b);
        s := s + b[1, 2];
        outreal(1, s)
    end
end
