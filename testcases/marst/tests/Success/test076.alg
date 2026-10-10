begin

    comment library 1;

    real
        x,
        y;
    integer
        i,
        j;
    array
        a[1 : 10];
    x := y := 1.0;
    i := j := 0;
    begin
        procedure p;
            begin
            x := 20;
            i := x + y;
            q(a[3], i + j)
            end;

        procedure q(l, k);
            value
                k;
            label
                l;
            integer
                k;
            if k = 0 then
                go to l;

        switch a :=
            a1,
            a2,
            a3;
        integer procedure j;
            j := y;

        real
            i;
        real procedure y;
            y := x;

        integer
            x;
        a1 :;
        x := 10;
        q(a[2], y - x);
        a2 :;
        p;
        if i = 0 | j = 0 | x = 1.0 | y = 1.0 then
            outreal(1, 100);
        a3 :;
    end;
    if i = 0 & j = 0 & x = 1.0 & y = 1.0 then
        outreal(1, 0)
end
