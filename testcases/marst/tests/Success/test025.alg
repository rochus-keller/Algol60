begin

    comment lib1;

    real
        x,
        y;
    integer
        i;
    for x := #6,
        #+6,
        #-6,
        .6,
        6,
        6.6,
        .6#6,
        .6#+6,
        .6#-6,
        6#6,
        6#+6,
        6#-6,
        6.6#6,
        6.6#+6,
        6.6#-6 do
        outreal(1, x);
    outreal(1, .0000000000000000123456789123456789);
    outreal(1, 3.1415926535897932384626433832795028841972);
    x := y := .0000001;
    for i := 1 step 1 until 13 do
        begin
        outreal(1, x);
        outreal(1, y);
        x := 10.0 * x;
        y := 110 * x / 10.0
        end
end
