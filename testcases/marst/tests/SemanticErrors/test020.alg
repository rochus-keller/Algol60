begin
comment library 1;
    array
        a[1 : 3],
        b[1 : 3, 1 : 3];
    real procedure sum(dim, a);
        value
            dim;
        integer
            dim;
        array
            a;
        sum :=
            if dim = 1 then
                a[1] + a[2] + a[3]
            else
                sum2(a);

    real procedure sum2(a);
        array
            a;
        sum2 := a[1, 1] + a[1, 2] + a[1, 3] + a[2, 1] + a[2, 2] +
            a[2, 3] + a[3, 1] + a[3, 2] + a[3, 3];

    integer
        i,
        j;
    for i := 1,
        2,
        3 do
        begin
        a[i] := i * i;
        for j := 1,
            2,
            3 do
            b[i, j] := a[i] + j
        end;
    outreal(1,sum(1, a));
    outreal(1,sum(2, b))
end
