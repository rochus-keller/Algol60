begin
comment library 1;
    integer
        i,
        p;
    i := 0;
    for p := 1 step 1 until 4 do
        begin
        own integer
            i;
        if p = 1 then
            i := 3
        else
            i := 2 * i;
        begin
            own integer
                j,
                i;
            if i = 0 then
                j := 4
            else
                j := 4 * i;
            outreal(1, j);
        end;
        i := 0;
        end;
    i := 0;
end
