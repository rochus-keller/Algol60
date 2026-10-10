begin
	integer a;

	outstring(1,"Enter an Integer");
	ininteger(0,a);

	if a>0 then outstring(1,"positive \n") else
	begin
	if a=0 then outstring(1,"zero \n") else outstring(1,"negative")
	end
end