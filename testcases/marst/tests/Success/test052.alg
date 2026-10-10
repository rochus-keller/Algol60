begin

    comment library 1;

    integer procedure iabs(i);
        value
            i;
        integer
            i;
        iabs :=
            if i < 0 then
                - i
            else
                i;

    integer
        i,
        j,
        k;
    i := j := k := 1;
    begin
        array
            a[(if iabs(j * k % i) > 3 * i - k then - 60 else 1) :
                (if iabs(j * k % i) > 3 * i - k then - 60 else 1)
                * 3.0 - 1.6 + 10.2],
            a2[(if iabs(j * k % i) > 3 * i - k then - 60 else 1)
                * 3.0 - 1.6 - 4 : (if iabs(j * k % i) > 3 * i - k
                then - 60 else 1), (if iabs(j * k % i) > 3 * i - k
                then - 60 else 1) * 3.0 - 1.6 - 4 : (if iabs(j * k
                % i) > 3 * i - k then - 60 else 1)];
        Boolean
            b;
        switch s :=
            l1,
            if (if iabs(j * k % i) > 3 * i - k then - 60 else 1)
                > 6 then l1 else l2,
            l2;
        a[(if iabs(j * k % i) > 3 * i - k then - 60 else 1)] :=
            a2[(if iabs(j * k % i) > 3 * i - k then - 60 else 1),
            (if iabs(j * k % i) > 3 * i - k then - 60 else 1)]
            := (if iabs(j * k % i) > 3 * i - k then - 60 else 1)
            * 3.0 - 1.6;
        b := (if iabs(j * k % i) > 3 * i - k then - 60 else 1) >
            6;
      l1 :;
        go to
            if (if iabs(j * k % i) > 3 * i - k then - 60 else 1)
                > 6 then
                l1
            else
                l2;
      l2 :;
        for i := 1 step (if iabs(j * k % i) > 3 * i - k then -
                60 else 1) until (if iabs(j * k % i) > 3 * i - k
                then - 60 else 1) do
            a[i] := (if iabs(j * k % i) > 3 * i - k then - 60
                else 1) * 3.0 - 1.6;
        for i := (if iabs(j * k % i) > 3 * i - k then - 60 else
                1),
            (if iabs(j * k % i) > 3 * i - k then - 60 else 1) +
                1,
            (if iabs(j * k % i) > 3 * i - k then - 60 else 1) + 10
                do
            a[i] := (if iabs(j * k % i) > 3 * i - k then - 60
                else 1);
        if (if iabs(j * k % i) > 3 * i - k then - 60 else 1) > 6
            then
            go to s[(if iabs(j * k % i) > 3 * i - k then - 60
                else 1) * 3.0 - 1.6]
        else
            for i := (if iabs(j * k % i) > 3 * i - k then - 60
                    else 1) step 10 until (if iabs(j * k % i) > 3
                    * i - k then - 60 else 1) - 1 do
                for j := (if iabs(j * k % i) > 3 * i - k then -
                        60 else 1) while (if iabs(j * k % i) > 3
                        * i - k then - 60 else 1) > 6 do
                   ;
        i := iabs((if iabs(j * k % i) > 3 * i - k then - 60 else
            1));
        j := iabs((if iabs(j * k % i) > 3 * i - k then - 60 else
            1) * 3.0 - 1.6);
        outreal(1, i + j + (if iabs(j * k % i) > 3 * i - k then -
            60 else 1) * 3.0 - 1.6 + (if iabs(j * k % i) > 3 * i -
            k then - 60 else 1) + a[1] + a2[1, 1]);
    end
end
