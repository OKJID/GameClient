ps.1.1
	tex t0
	tex t1
	texbem t2, t1 ; use t1 as env map adjustment on t2.
	mul r0,v0,t0 ; blend vertex color into t0.
	mul r1.rgb,t2,c0 ; reduce t2 (environment mapped reflection) by constant
	add r0.rgb, r0, r1
