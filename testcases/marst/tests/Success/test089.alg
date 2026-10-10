begin

    comment library 1;

    real
        i,
        j;
    array
        a[1 : 2, 1 : 3];
    real procedure gps(i, n, z, v);
        real
            i,
            n,
            z,
            v;
        begin
        for i := 1 step 1 until n do
            z := v;
        gps := 1
        end;

    outreal(1, gps(j, 3.0, i, gps(i, 2.0, a[i, j], i + j)));
    for i := 1 step 1 until 2 do
        for j := 1 step 1 until 3 do
            if a[i, j] != i + j then
                outreal(1, 11 * i + j)
end
