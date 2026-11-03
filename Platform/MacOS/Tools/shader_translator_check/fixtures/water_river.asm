ps.1.1
	tex t0
	tex t1
	tex t2
	tex t3
	mul r0.rgb, v0, t0 ; blend vertex color into t0.
	mov r0.a, t0 ; keep vertex alpha from fading the base water.
	mul r1, t1, t2 ; mul
	add r1.rgb, r1, t3
	mul r1.rgb, r1, v0.a
	+mul r0.a, r0, t3
	add r0.rgb, r0, r1
