begin
    comment library 1;
    integer
        i,
        j,
    sum;
    i := 1;
    j := 2;
    sum := 1 + (if i > j then 0 else (if i > j then 0 else
        (if i > j then 0 else (if i > j then 0 else (if i
        > j then 0 else (if i > j then 0 else (if i > j
        then 0 else (if i > j then 0 else (if i > j then
        0 else 1)))))))));
    outreal(1, sum);
end
