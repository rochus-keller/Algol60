begin
	integer n;
	procedure p(k); value k; integer k; comment if value not passed 5;
	begin
		n:=n+1;
		k:=k+4;
		outinteger(1,n);
	end;

	n := 0;
	p(n);
	outinteger(1,n);
end