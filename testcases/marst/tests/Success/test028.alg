begin

    comment lib 1;

    integer
        n1,
        n2;
    procedure dummy;
        ;

    procedure p(x, l);
        value
            x;
        integer
            x;
        procedure
            l;
        begin
        procedure gotol1;
            go to l1;

        if x = n1 then
            p(x - 1, gotol1)
        else if x = n2 then
            l
        else
            p(x - 1, l);
        l1 :;
        outreal(1, x)
        end p;

    n1 := 4;
    n2 := 2;
    p(5, dummy);
    n2 := 0;
    p(7, dummy);
end
