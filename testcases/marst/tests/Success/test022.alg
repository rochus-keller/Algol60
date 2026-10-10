begin
comment lib 1;
    procedure recursive(level, l);
        value
            level;
        integer
            level;
        label
            l;
        begin
        if level != 0 then
            recursive(level - 1, exit)
        else
            go to l;
       exit :;
        outreal(1,level)
        end;

    recursive(10, exit);
   exit :;
end
