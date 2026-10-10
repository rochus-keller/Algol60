begin
comment lib 1;
    integer array
        a[1 : 9];
    integer
        i,
        s;
    procedure dummy(a);
        value
            a;
        array
            a;
        sum(a);

    procedure sum(a);
        array
            a;
        for i := 1 step 1 until 9 do
            begin
            s := s + a[i];
            a[i] := 0
            end;

    s := 0;
    for i := 1 step 1 until 9 do
        a[i] := i;
    dummy(a);
    outreal(1,s);
    for i := 1 step 1 until 9 do
        if a[i] != i then
            outreal(1,i)
end
