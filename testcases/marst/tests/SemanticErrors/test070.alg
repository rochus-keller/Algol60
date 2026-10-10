begin

    comment library 1;

    integer
        i;
    switch s :=
        s1,
        s2,
        s3;
    i := 1;
    go to s[i];
    for i := 1,
        2,
        3 do
        begin
        s1 :;
        outreal(1, 10);
        s2 :;
        outreal(1, 20);
        outreal(1, i)
        end;
    s3 :;
    outreal(1, 30)
end
