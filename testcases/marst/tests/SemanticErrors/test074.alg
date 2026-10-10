begin

    comment library 1;

    integer
        n;
    n := 3;
    begin
        array
            a[1 : if n = 3 then 3 else - 100];
        integer
            n;
        n := 2;
        outreal(1, n);
    end;
    outreal(1, n)
end
