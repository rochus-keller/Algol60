begin
comment lib5;
    integer
        i;
    Boolean
        b;
    procedure labelbyvalue(l);
        value
            l;
        label
            l;
        ;

    procedure labelbyname(l);
        label
            l;
        begin
        i := 1;
        if b then
            go to l
        end;

    switch s :=
        l1,
        l2;
    integer procedure nottouse;
        nottouse := sqrt( - 1);

    integer procedure one;
        one := 1;

    b := false;
    labelbyname(s[nottouse]);
    b := true;
    i := 2;
    labelbyname(s[i]);
   l1 :;
    outreal(1,1);
   l2 :;
    outreal(1,2);
    labelbyvalue(s[one]);
end
