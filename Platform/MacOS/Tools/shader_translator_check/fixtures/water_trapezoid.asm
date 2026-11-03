ps.1.1
	tex t0 ;get water texture
	tex t1 ;get white highlights on black background
	tex t2 ;get white highlights with more tiling
	tex t3	; get black shroud
	mul r0,v0,t0 ; blend vertex color and alpha into base texture.
	mad r0.rgb, t1, t2, r0	; blend sparkles and noise
	mul r0.rgb, r0, t3 ; blend in black shroud
	;
