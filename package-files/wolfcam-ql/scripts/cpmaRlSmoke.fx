// something like cpma rl smoke
//
// /set cg_fxfile "scripts/cpmaRlSmoke.fx"
// /fxload


// cpma'ish rl smoke trail
// input: origin, angles, velocity, dir (normalized), axis
//
// no client info for rocket trails
weapon/rocket/trail {
	color	1 0.75 0
	size	200
	Light

	color	1 1 1
	//alpha	0.33
	alpha	0.5
	shader	smokePuff
	rotate	360 * rand
	//distance 145 {
	interval 0.050 {
		t0 80 + crand * 5
		t1 crand * 5
		t2 crand * 5
		clear velocity
		emitter 2 {
			origin0 origin0 + t1
			origin1 origin1 + t2
			alphaFade	0.2
			size		20
			moveGravity t0
			sprite		cullNear
		}
	}
}
