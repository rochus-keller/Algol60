begin

    comment library 1;

    Boolean
        assign;
    procedure p(x);
        real
            x;
        begin
        outreal(1, x);
        if assign then
            x := 3.14;
        end;

    real
        x,
        y;
    assign := false;
    p(1.0);
    x := y := 2.0;
    p(x + y);
    assign := true;
    p(x);
    p(1.0);
    outreal(1, 1.0)
end
