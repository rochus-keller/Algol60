begin

    comment library 1;
    integer
        i,
        j,
        k,
        case;

    procedure equal(i, j);
        value
            i,
            j;
        integer
            i,
            j;
        if i != j then
            begin
            outreal(1, case);
            outreal(1, i);
            outreal(1, j)
            end
        else
            case := case + 1;

    i := 2;
    j := 5;
    k := 9;
    case := 0;
    equal(i + j + k, 16);
    equal(i + j - k, - 2);
    equal(i + j * k, 47);
    equal(i + j % k, 2);
    equal(i + j^k, 1953127);
    equal(i - j + k, 6);
    equal(i - j - k, - 12);
    equal(i - j * k, - 43);
    equal(i - j % k, 2);
    equal(i - j^k, -1953123);
    equal(i * j + k, 19);
    equal(i * j - k, 1);
    equal(i * j * k, 90);
    equal(i * j % k, 1);
    equal(i * j^2, 50);
    equal(j % i + k, 11);
    equal(j % i - k, - 7);
    equal(j % i * k, 18);
    equal(j % i % k, 0);
    equal(j % i^k, 0);
    equal(i^j + k, 41);
    equal(i^j - k, 23);
    equal(i^j * k, 288);
    equal(i^j % k, 3);
    equal(i^j^2, 1024);
    outreal(1, case)
end
