
weapon/rail/trail {
        // like quake2 rail core
	copy dir v3

	t0 v3
	normalize dir

	shader railDisc
	//shader gfx/misc/tracer

	repeat (t0 / 0.75) {
	       size 0.5 + crand * 0.4
	       red 0.5 + crand * 0.28
	       green 0.3
	       blue 1

	       addScale v1 dir origin loop * t0
	       add parentOrigin origin origin
	       origin0 origin0 + crand * 3
	       origin1 origin1 + crand * 3
	       origin2 origin2 + crand * 3
	       velocity0 crand * 3
	       velocity1 crand * 3
	       velocity2 crand * 3
	       emitter 1 {
	       	       moveGravity 0
		       ColorFade 0.7
	       	       Sprite
	       }
	}
}
