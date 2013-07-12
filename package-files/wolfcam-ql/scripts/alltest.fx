
// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/lightning/impact {
	// random impact sound

        // wolfcam:  enable quake3 sound settings before playing impact sound
        // so that the sound isn't fps dependent
        t0 s_maxSoundInstances
        t1 s_maxSoundRepeatTime
        command s_maxSoundInstances -1
        command s_maxSoundRepeatTime 50
	soundList {
		sound/weapons/lightning/lg_hit.wav
		sound/weapons/lightning/lg_hit2.wav
		sound/weapons/lightning/lg_hit3.wav
	}
        command s_maxSoundInstances %t0
        command s_maxSoundRepeatTime %t1

	// Hole mark shader decal
	shader	gfx/damage/hole_lg_mrk
	size	6
	//decal
        sprite

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	//color	0.3 0.3 0.9
	color	0.5 0.6 1.0
	repeat 3 {
		wobble	dir velocity 10 + rand*20
		scale	velocity velocity 200 + rand*50
		size	2 + rand
                t0 size
		emitter 20 {  //"0.6 + rand*0.3" {
                        size (t0 * (1.0 - lerp))
			//moveGravity 300
			moveBounce 300 0.7
			colorFade 0.3  // 0.7
			Sprite
		}
	}
}

moveup {
    velocity0 150 * rand
    velocity1 150 * rand
    velocity2 200 + 600 * rand
    red rand
    green rand
    blue rand
}
