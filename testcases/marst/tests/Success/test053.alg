begin

    comment library5;

    integer
        i,
        j;
    for i := - 2,
         - 1,
        1,
        2 do
        for j := - 2 step 1 until 2 do
            if j % i != sign(j / i) * entier(abs(j / i))
                then
                begin
                outreal(1, j);
                outreal(1, i);
                outreal(1, j % i)
                end;
end
