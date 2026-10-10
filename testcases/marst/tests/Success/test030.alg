begin

    comment lib 1;

    integer
        n1,
        n2;

    procedure p(x, l);
        value
            x;
        integer
            x;
        label
            l;
        begin

        if x = n1 then
            p(x - 1, l1)
        else if x = n2 then
            go to l
        else
            p(x - 1, l);
        l1 :;
                outreal(1, x)
        end p;

    n1 := 4;
    n2 := 2;
    p(5, l2);
    n2 := 0;
    p(7, l2);

    l2:;
end
