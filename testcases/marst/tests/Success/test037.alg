begin
 integer
  a0,
  a1,
  a2,
  a3,
  a4,
  a5,
  a6,
  a7,
  a8,
  a9,
  a10,
  a11;
 integer
  sum;

 comment library 1;


 sum := 0;
 for a0 := 1,
  2 do
  begin
  sum := sum + 1;
  for a1 := 1,
   2 do
   begin
   sum := sum + 1;
   for a2 := 1,
    2 do
    begin
    sum := sum + 1;
    for a3 := 1,
     2 do
     begin
     sum := sum + 1;
     for a4 := 1,
      2 do
      begin
      sum := sum + 1;
      for a5 := 1,
       2 do
       begin
       sum := sum + 1;
       for a6 := 1,
        2 do
        begin
        sum := sum + 1;
        for a7 := 1,
         2 do
         begin
         sum := sum + 1;
         for a8 := 1,
          2 do
          begin
          sum := sum + 1;
          for a9 := 1,
           2 do
           begin
           sum := sum + 1;
           for a10 := 1,
            2 do
            begin
            sum := sum + 1;
            for a11 := 1,
             2 do
             begin
             sum := sum + 1;
             end
            end
           end
          end
         end
        end
       end
      end
     end
    end
   end
  end;

 outreal(1, sum);

end
