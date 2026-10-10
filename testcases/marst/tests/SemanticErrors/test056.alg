begin

    comment library5;

    integer
        i,
        j,
        case;
    real
        x,
        y;
    procedure requal(a, b);
        value
            a,
            b;
        real
            a,
            b;
        if a != b then
            begin
            outreal(1, a);
            outreal(1, b);
            outreal(1, case)
            end
        else
            case := case + 1;

    case := 1;
    x := 2.0;
    y := 3.0;
    i := 2;
    j := 3;
    requal(x^i, 4.0);
    if i^j != 8 then
        begin
        outreal(1, i^j);
        outreal(1, 8);
        end;
    requal(x^0, 1.0);
    if i^0 != 1 then
        begin
        outreal(1, i^0);
        outreal(1, 1)
        end;
    requal(x^( - i), 1.0 / (2.0 * 2.0));
    requal(i^( - i), 1.0 / (2.0 * 2.0));
    requal(x^y, exp(y * ln(x)));
    requal(j^y, exp(y * ln(j)));
    requal(0.0^x, 0.0);
    requal(0^x, 0.0);

    comment   should fail;

    requal(( - x)^y, - 1)
end
