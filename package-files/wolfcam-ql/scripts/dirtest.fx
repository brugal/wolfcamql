
// input: origin, dir
//
// no client info for rocket impact

weapon/rocket/impact {
        copy velocity v0
	//copy dir v2
	//echo velocity2
	//command one two three
	//command "four five six" seven

	vibrate 70
	sound	sound/weapons/rocket/rocklx1a.wav
	// Mark on the wall, using direction from parent
	shader gfx/damage/burn_med_mrk
	size	64
	Decal

	// Animating sprite of the explosion
	shader rocketExplosion
	size 40
	// Will be the light colour
	color 1 0.75 0
	emitter 1 {
		Sprite //depthhack
		//copy velocity dir
		//Decal
		// size will goto zero after 0.5 of the time
		size 300 * clip(2 - 2*lerp)
		Light
	}

	//copy v2 dir

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}

// input: origin, dir
//
// no client info
weapon/bfg/impact {
        copy velocity v0

	vibrate 100
	sound	sound/weapons/rocket/rocklx1a.wav
	// Mark on the wall
	shader	gfx/damage/burn_med_mrk
	size	32
	decal

	// Animating sprite of the explosion
	shader bfgExplosion
	size 40
	emitter 0.6 {
		sprite
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

//////////////////////
	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}

// input: origin, dir, surfacetype
//
// no client info
weapon/grenade/impact {
  //origin2 origin2 + 5
        copy velocity v0

	vibrate 70
	sound	sound/weapons/rocket/rocklx1a.wav
	// Mark on the wall, using direction from parent
	shader gfx/damage/burn_med_mrk
	size	64
	Decal

	// Animating sprite of the explosion
	shader grenadeExplosion
	size 40
	// Will be the light colour
	color 1 0.75 0
	emitter 0.6 {
		Sprite
		// size will goto zero after 0.5 of the time
		size 300 * clip(2 - 2*lerp)
		Light
	}

/////////////////////////////

	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		//copy dir v1
		//scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			//moveGravity 0  //300
			moveBounce 0 1  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			//moveGravity 0  //300
			moveBounce 0 1  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}

// input: origin, dir, surfacetype
//
// no client info
weapon/plasma/impact {
        copy velocity v0

	vibrate 10
	sound	sound/weapons/plasma/plasmx1a.wav

	shader	plasmaExplosion
	model	models/weaphits/ring02.md3
	// Show the ring impact model on a random rotation around it's direction
	rotate    rand*360
	emitter 0.6 {
		dirModel
	}
	size	24
	shader	gfx/damage/plasma_mrk
	Decal energy

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

/////////////////////////////
	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/rail/impact {
        copy velocity v0

	vibrate 50
	sound	sound/weapons/plasma/plasmx1a.wav

	// The white expanding impact disc
	rotate    rand*360
	shader	railExplosion
	model	models/weaphits/ring02.md3
	emitter 0.6 {
		dirModel
	}
	// Plasma style impact mark
	size	24
	shader	gfx/damage/plasma_mrk
	Decal energy

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

///////////////////////////////
	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/shotgun/impact {
        copy velocity v0

	vibrate 1
	// Bullet mark on the wall, shotgun ones are smaller
	shader		gfx/damage/bullet_mrk
	size		4
	Decal

	// explosion cone with animating shader
	size		1
	shader		bulletExplosion
	model		models/weaphits/bullet.md3
	rotate		rand*360
	emitter 0.6 {
		dirModel
	}

	// a single sprite shooting up from impact surface
	wobble	dir velocity 10 + rand*30
	scale	velocity velocity 200 + rand*50
	size	1 + rand*0.5
	shader	flareShader
	alpha	0.8
	color	1 0.6 0.3
	emitter 0.6 + rand*0.3 {
		moveGravity 200
		colorFade 0.6
		Sprite
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

/////////////////////////////////////////
	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}


weapon/chaingun/impact {
        copy velocity v0

	soundList {
		sound/weapons/machinegun/ric1.wav
		sound/weapons/machinegun/ric2.wav
		sound/weapons/machinegun/ric3.wav
	}
	// Bullet mark on the wall
	shader gfx/damage/bullet_mrk
	size 8
	decal

	// explosion cone with animating shader
	size		1
	shader		bulletExplosion
	model		"models/weaphits/bullet.md3"
	rotate		rand*360
	emitter 0.6 {
		dirModel
	}

	// a single sprite shooting up from impact surface
	wobble	dir velocity 10 + rand*30
	scale	velocity velocity 200 + rand*50
	size	2 + rand
	shader	flareShader
	alpha	0.8
	color	1 0.75 0.6
	emitter "0.6 + rand*0.3" {
		moveGravity 200
		colorFade 0.6
		Sprite
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

///////////////////////////////

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/machinegun/impact {
        copy velocity v0

	//command echo one two three
	//command echo "four five six" seven
	//command fxmath v00 + 5
	//command echo hello %v00 xxx
	//command echopopup hello %v00 %% %%%v01
        //t0 100 + rand * 20
        //command set cg_fov %t0


	soundList {
		sound/weapons/machinegun/ric1.wav
		sound/weapons/machinegun/ric2.wav
		sound/weapons/machinegun/ric3.wav
	}
	// Bullet mark on the wall
	shader gfx/damage/bullet_mrk
	size 8
	decal

	// explosion cone with animating shader
	size		1
	shader		bulletExplosion
	model		"models/weaphits/bullet.md3"
	rotate		rand*360
	emitter 0.6 {
		dirModel
	}

	// a single sprite shooting up from impact surface
	wobble	dir velocity 10 + rand*30
	scale	velocity velocity 200 + rand*50
	size	2 + rand
	shader	flareShader
	alpha	0.8
	color	1 0.75 0.6
	emitter "0.6 + rand*0.3" {
		moveGravity 200
		colorFade 0.6
		Sprite
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}

////////////////////////////////

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/lightning/impact {
        copy velocity v0

	// random impact sound
	soundList {
		sound/weapons/lightning/lg_hit.wav
		sound/weapons/lightning/lg_hit2.wav
		sound/weapons/lightning/lg_hit3.wav
	}
	// Hole mark shader decal
	shader	gfx/damage/hole_lg_mrk
	size	12
	decal

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite //depthhack
		}
	}
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/nailgun/impact {
        copy velocity v0

	//command echo one two three
	//command echo "four five six" seven
	//command fxmath v00 + 5
	//command echo hello %v00 xxx
	//command echopopup hello %v00 %% %%%v01

	soundList {
		sound/weapons/machinegun/ric1.wav
		sound/weapons/machinegun/ric2.wav
		sound/weapons/machinegun/ric3.wav
	}
	// Bullet mark on the wall
	shader gfx/damage/bullet_mrk
	size 8
	decal

	// explosion cone with animating shader
	size		1
	shader		bulletExplosion
	model		"models/weaphits/bullet.md3"
	rotate		rand*360
	emitter 0.6 {
		dirModel
	}

	// a single sprite shooting up from impact surface
	wobble	dir velocity 10 + rand*30
	scale	velocity velocity 200 + rand*50
	size	2 + rand
	shader	flareShader
	alpha	0.8
	color	1 0.75 0.6
	emitter "0.6 + rand*0.3" {
		moveGravity 200
		colorFade 0.6
		Sprite
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 3 {
		//wobble	dir velocity 10 + rand*20
		copy dir velocity
		scale	velocity velocity 200 + rand*50
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}

////////////////////////////////

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy dir v1
		scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}


weapon/prox/impact {
  //origin2 origin2 + 5
        copy velocity v0

	vibrate 70
	sound	sound/weapons/rocket/rocklx1a.wav
	// Mark on the wall, using direction from parent
	shader gfx/damage/burn_med_mrk
	size	64
	Decal

	// Animating sprite of the explosion
	shader grenadeExplosion
	size 40
	// Will be the light colour
	color 1 0.75 0
	emitter 0.6 {
		Sprite
		// size will goto zero after 0.5 of the time
		size 300 * clip(2 - 2*lerp)
		Light
	}

/////////////////////////////

	shader	flareShader
	alpha	0.8
	color	0.3 0.3 0.9
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		//copy dir v1
		//scale v1 dir -1
		copy dir velocity
		//copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			//moveBounce 0 1  //300
			colorFade 0.7
			Sprite
		}
	}

	// a single sprite shooting up from impact surface
	shader	flareShader
	alpha	0.8
	color	0.9 0.3 0.3
	repeat 20 {
		//wobble	dir velocity 10 + rand*20
		copy v0 velocity
		scale	velocity velocity 100 + rand*100
		size	2 + rand
		emitter "0.6 + rand*0.3" {
			moveGravity 0  //300
			//moveBounce 0 1  //300
			colorFade 0.7
			Sprite depthhack
		}
	}
}
