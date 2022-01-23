.text 
addo_test:
	ldconst 1, g1
	ldconst 2, g0 
	addo g1, g0, g2
	cmpo g2, 3
	faultne

subo_test:
	ldconst 1, g1
	ldconst 2, g0 
	subo g1, g0, g2
	cmpo g2, 1
	faultne

mulo_test:
	ldconst 3, g1
	ldconst 2, g0 
	mulo g1, g0, g2
	cmpo g2, 6
	faultne

addi_test:
	ldconst -3, g0
	ldconst -4, g1
	ldconst -7, g2
	addi g1, g0, g3
	cmpi g3, g2
	faultne 

subi_test:
	ldconst -3, g0
	ldconst -4, g1
	subi g1, g0, g2
	cmpi g3, 1
	faultne

muli_test:
	ldconst -3, g0
	ldconst -4, g1
	muli g1, g0, g2
	cmpi g3, 12
	faultne

