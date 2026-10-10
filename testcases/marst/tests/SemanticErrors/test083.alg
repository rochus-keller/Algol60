begin

    comment library 1;

    integer
        nn;
    switch a :=
        b[1],
        b[2];
    switch b :=
        a[g],
        a[2];
    integer procedure f(n, s);
        value
            n;
        integer
            n;
        switch
            s;
        begin
        nn := n;
        go to s[1];
        f := nn
        end;

    integer procedure g;
        begin
        integer
            n;
        n := nn;
        g := 0;
        nn :=
            if n <= 1 then
                n
            else
                f(n - 1, a) + f(n - 2, a)
        end;

    outreal(1, f(20, a))
end
