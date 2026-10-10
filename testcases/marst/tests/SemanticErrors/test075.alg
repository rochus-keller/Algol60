begin

    comment library 9;


    integer array
        eq[1 : 3];
    integer
        i;
    real
        x,
        y;
    procedure inc(a, b);
        value
            a,
            b;
        real
            a,
            b;
        if a < b then
            eq[1] := eq[1] + 1
        else if a = b then
            eq[2] := eq[2] + 1
        else
            eq[3] := eq[3] + 1;



    for i := 1 step 1 until 3 do
        eq[i] := 0;
    for i := 2 step 1 until 100 do
        begin
        inreal(2, x);
        y := 1 / (i * i - 1);
        outreal(1, y);
        inc(x, y)
        end;
    for i := 1 step 1 until 3 do
        outreal(1, eq[i]);


end
