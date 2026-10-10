begin

    comment library 1;

    integer
        i;
    procedure p;
        begin
        integer
            j;
        procedure q;
            if i > 10 then
                go to l;

        j := i := i + 1;
        q;
        outreal(1, j);
        p;
        l :;
        outreal(1, j);
        end;

    i := 1;
    p;
end
