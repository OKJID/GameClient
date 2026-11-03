ps.1.4
texld r0, t0
mov r1.a, r0.r
mov r2.a, r0.g
mov r3.a, r0.b
mul r0.rgb, r3.a, c0
mad r0.rgb, r2.a, c1, r0
mad r0.rgb, r1.a, c2, r0
