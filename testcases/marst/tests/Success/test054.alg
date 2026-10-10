begin

    comment library5;

    integer procedure tsign(x);
        value
            x;
        real
            x;
        tsign :=
            if x > 0 then
                1
            else if x < 0 then
                 - 1
            else
                0;

    integer procedure tentier(x);
        value
            x;
        real
            x;
        tentier := x - .5;

    real procedure tabs(x);
        value
            x;
        real
            x;
        tabs :=
            if x < 0 then
                 - x
            else
                x;

    real
        x;
    for x := - 100,
         - 100.1,
         - 99.4,
         - .4,
         - .1,
        0,
        .1,
        .4,
        .6,
        300 do
        if tsign(x) != sign(x) | tentier(x) != entier(x) |
            tabs(x) != abs(x) then
            outreal(1, x)
end
