.text 
	ldconst 1, g1
	ldconst 2, g0 
	addo g1, g0, g2
	cmpo g2, 3
	faultne
	subo g1, g0, g2
	cmpo g2, 1
	faultne
	ldconst -3, g0
	ldconst -4, g1
	ldconst -7, g2
	addi g1, g0, g3
	cmpi g3, g2
	faultne 
	subi g1, g0, g2
	cmpi g3, 1
	faultne
