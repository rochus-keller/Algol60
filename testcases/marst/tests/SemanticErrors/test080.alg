begin

    comment library 1;

    integer
        i;
    integer array
        a[1 : 10];
    i := 1;
    for a[i] := 1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10 do
        i := i + 1;
    for i := 1 step 1 until 10 do
        if a[i] != i then
            outreal(1, i);
    outreal(1, 0)
end
