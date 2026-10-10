begin

    comment library 1;

    integer
        sum;
    procedure p1(s, r, i, b, ra, ia);
        string
            s;
        real
            r;
        integer
            i;
        Boolean
            b;
        real array
            ra;
        integer array
            ia;
        sum := sum + r + i + (if b then 1 else 0) + ra[1] +
            ia[1];

    procedure p2(ba, l, sw, pr, rpr, ipr, bpr);
        Boolean array
            ba;
        label
            l;
        switch
            sw;
        procedure
            pr;
        real procedure
            rpr;
        integer procedure
            ipr;
        Boolean procedure
            bpr;
        begin
        pr;
        sum := sum + rpr + ipr;
        if bpr then
            go to l;
        if ba[1] then
            go to sw[2]
        end;

    procedure p3(r, i, b, ra, ia, ba, l);
        value
            r,
            i,
            b,
            ra,
            ia,
            ba,
            l;
        real
            r;
        integer
            i;
        Boolean
            b;
        real array
            ra;
        integer array
            ia;
        Boolean array
            ba;
        label
            l;
        begin
        sum := sum + r + i + ra[1] + ia[1] + (if b then 1
            else 0) + (if ba[1] then 1 else 0);
        if ba[1] then
            go to l
        end;

    real
        r;
    Boolean
        b;
    real array
        ra[1 : 1];
    integer array
        ia[1 : 1];
    Boolean array
        ba[1 : 1];
    integer procedure ipr;
        ipr := 2 * sum;

    real procedure rpr;
        rpr := 3 * sum;

    Boolean procedure bpr;
        bpr := ! b;

    procedure pr;
        sum := 5 * sum + 1;

    switch ss :=
        work,
        fail;
    sum := 0;
    ia[1] := - 2;
    ra[1] := 3.55;
    ba[1] := false;
    b := true;
    r := 6.4;
    p1( "ab", r, sum, b, ra, ia);
    if sum != 9 then
        outreal(1, sum);
    p2(ba, ss[2], ss, pr, rpr, ipr, bpr);
    if sum != 276 then
        outreal(1, sum);
    work :;
    outreal(1, 0);
    p3(4.6, - 200, true, ra, ia, ba, if true then ss[2] else
        fail);
    if sum != 83 then
        outreal(1, sum);
    p1( "cd", rpr, ipr, bpr, ra, ia);
    if sum != 500 then
        outreal(1, sum);
    p3(rpr, ipr, bpr, ra, ia, ba, fail);
    if sum != 3002 then
        outreal(1, sum);
    if false then
        fail : outreal(1, - 1);
end
