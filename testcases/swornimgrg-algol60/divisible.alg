begin comment count divisible by 2 from 1 to 50;
	integer i;
	integer count;

	count := 0;

	for i:=1 step 1 until 50 do
	begin
		if i/2*2 = i then count :=count +1
	end;

	comment fixed by RK;
	outstring(1,"This many numbers between 1 & 50 are divisible by 2: ");
	outinteger(1,count); newline
end
