begin

    comment library 1;

    procedure p(f);
        real procedure
            f;

        comment   (f1):label f1;

        f((m));

    b1 :;
    begin
        real procedure q(l);
            label
                l;
            begin
            go to l;
            q := 0;
            outreal(1, 1)
            end;

        b2 :;
        begin
            real
                i;
            go to n;
            i := 2;
            outreal(1, i)
        end;
        n :;
        p(q);
        outreal(1, 3);
    end b1;
    outreal(1, 4);
    m :;
    outreal(1, 5)
end
