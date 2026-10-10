begin
	integer array a[1:10];
	integer array b[-10:10,1:10];
	real array c[-10:0];
	integer i;

	for i:=1 step 1 until 10 do 
	begin a[i]:= i*i ; outinteger(1,a[i]) end;

	for i:= 1,2,3,4,5,6,7,8,9,10 do 
	begin a[i]:= i*i*i ; outinteger(1,a[i]) end;

	i:=1;
	for i:=i+1 while i<=10 do
	begin a[i]:= i*i*i*i; outinteger(1,a[i]) end;

end