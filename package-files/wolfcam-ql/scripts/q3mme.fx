// q3mme example scripts combined into one file with example changes to make
// it compatible with wolfcam

// wolfcam additional inputs to some scripts are:
//    team         0 free, 1 red, 2 blue, 3 spectator
//    clientnum
//    enemy        1 if enemy of whoever is being specced in demo
//    teammate
//    ineyes       1 if first person view is with this player
//    surfacetype  for impact scripts:  0 not specified, 1 metal, 2 wood,
//                   3 dust, 4 snow
//

/////////////////////////////////////////////////////


// Some general fx script you could use in the effects editor when really bored
mme/decal {
	Trace
	decalTemp
}

mme/quad {
	quad
}


mme/model {
	anglesModel
}

// Talk sprite
// input: origin, team, clientnum, enemy, teammate, ineyes
player/talk {
        if ineyes != 1 {
        	size		10
	        shader		sprites/balloon3
                Sprite
        }
}

// Impressive sprite
// input: origin, team, clientnum, enemy, teammate, ineyes
player/impressive {
        if ineyes != 1 {
        	size		10
	        shader		medal_impressive
                Sprite
        }
}

// Excellent sprite
// input: origin, team, clientnum, enemy, teammate, ineyes
player/excellent {
        if ineyes != 1 {
        	size		10
                shader		medal_excellent
                Sprite
        }
}

// Holy shit medal from ra3
// input: origin, team, clientnum, enemy, teammate, ineyes
player/holyshit {
        if ineyes != 1 {
        	size		10
                shader		medal_holyshit
                Sprite
        }
}

// Accuracy medal
// input: origin, team, clientnum, enemy, teammate, ineyes
player/accuracy {
        if ineyes != 1 {
        	size		10
                shader		medal_accuracy
                Sprite
        }
}

// Gauntlet sprite
// input: origin, team, clientnum, enemy, teammate, ineyes
player/gauntlet {
        if ineyes != 1 {
        	size		10
	        shader		medal_gauntlet
                Sprite
        }
}

// connection sprite
// input: origin, team, clientnum, enemy, teammate, ineyes
player/connection {
        if ineyes != 1 {
        	size		10
	        shader		disconnected
                Sprite
        }
}

// haste/speed trail
// input: origin, angles, velocity, dir (normalized), team, clientnum,
//        enemy, teammate, ineyes
player/haste {
	interval 0.1 {
		shader		hasteSmokePuff
		emitter 0.5 {
			size	6 + 10 * wave( 0.5  * lerp )
			alphaFade	0
			sprite	cullNear
		}
	}
}

// flight trail -- wolfcam addition, not in q3mme
// input: origin, angles, velocity, dir (normalized), team, clientnum,
//        enemy, teammate, ineyes
player/flight {
	interval 0.1 {
		shader		hasteSmokePuff
		emitter 0.5 {
			size	6 + 10 * wave( 0.5  * lerp )
			alphaFade	0
			sprite	cullNear
		}
	}
}

// wolfcam addition, not in q3mme:
//
// player/head/trail
// player/torso/trail
// player/legs/trail
//
// input: origin, angles, velocity, dir (normalized), team, clientnum,
//        enemy, teammate, ineyes

//player/legs/trail {
//	interval 0.1 {
//		shader		sprites/balloon3
//		emitter 0.5 {
//			size	6 + 10 * wave( 0.5  * lerp )
//			alphaFade	0
//			sprite	cullNear
//		}
//	}
//}


// teleport in effect and sound effect
// input: origin
player/teleportIn {
	sound	sound/world/telein.wav
        // Show the fading spawn tube for 0.5 seconds
	//model models/misc/telep.md3
        // wolfcam ql uses different model
        model models/powerups/pop.md3
	//shader teleportEffect  // wolfcam for ql don't add shader
	emitter 0.5 {
		colorFade 0
		anglesModel
	}
}

// teleport out effect and sound effect
// input: origin
player/teleportOut {
	sound	sound/world/teleout.wav
        //Show the fading spawn tube for 0.5 seconds
	//model	models/misc/telep.md3
        // wolfcam ql uses different model
        model models/powerups/pop.md3
	//shader	teleportEffect  // wolfcam for ql don't add shader
	emitter 0.5 {
		colorFade 0
		anglesModel
	}
}


// Explode the body into gibs
// input: origin, velocity, team, clientnum, enemy, teammate, ineyes
//
// wolfcam: guessing that velocity corresponds to player
player/gibbed {
  if cl_useq3gibs {
        sound sound/player/gibsplt1.wav
	repeat 11 {
		//Huge list of different body parts that get thrown away
		modellist loop {
                        // wolfcam gibs located in different directory
			//models/gibs/abdomen.md3
			//models/gibs/arm.md3
			//models/gibs/chest.md3
			//models/gibs/fist.md3
			//models/gibs/foot.md3
			//models/gibs/forearm.md3
			//models/gibs/intestine.md3
			//models/gibs/leg.md3
			//models/gibs/leg.md3
			//models/gibs/skull.md3
			//models/gibs/brain.md3

			models/gibsq3/abdomen.md3
			models/gibsq3/arm.md3
			models/gibsq3/chest.md3
			models/gibsq3/fist.md3
			models/gibsq3/foot.md3
			models/gibsq3/forearm.md3
			models/gibsq3/intestine.md3
			models/gibsq3/leg.md3
			models/gibsq3/leg.md3
			models/gibsq3/skull.md3
			models/gibsq3/brain.md3
		}

		// aim v0 in random direction, giving more height to upwards vector and scale add to velocity
		random		v0
		v02			v02+1
		addScale	parentVelocity v0 velocity 350
		yaw			360*rand
		pitch		360*rand
		roll		360*rand
		emitter 5 + rand*3 {
			sink 0.9 50
			moveBounce 800 0.6
                        // wolfcam  this is called after impact and distance
			//anglesModel
			impact 50 {
                                // wolfcam  need to add gib splat sounds yourself
                                if rand < 0.5 {
                                    t0 rand * 3
                                    if t0 < 1 {
                                        sound sound/player/gibimp1.wav
                                    } else if t0 < 2 {
                                        sound sound/player/gibimp2.wav
                                    } else {
                                        sound sound/player/gibimp3.wav
                                    }
                                }
				size 16 + rand*32
				//shader bloodMark
                                // wolfcam
                                shader q3bloodMark
				decal alpha
			}
			distance 20 {
				// Slowly sink downwards
				clear velocity
				velocity2 -10
				rotate rand*360
				//shader bloodTrail
                                // wolfcam
                                shader q3bloodTrail
				emitter 2 {
					moveGravity 0
					alphaFade 0
					rotate rotate + 10*lerp
					size  8 + 10*lerp
					sprite
				}
			}
                        anglesModel  // wolfcam  moved down here
		}
	}
    } else {
           t0 rand * 4
           if t0 < 1 {
               sound sound/misc/electrogib_01.ogg
	   } else if t0 < 2 {
	       sound sound/misc/electrogib_02.ogg
	   } else if t0 < 3 {
               sound sound/misc/electrogib_03.ogg
	   } else {
               sound sound/misc/electrogib_04.ogg
	   }

           // death effect
           shader deathEffect
           size 100
           copy velocity v3
           clear velocity
           emitter 0.5 {
                   sprite
           }
           copy v3 velocity
           shaderClear

           repeat 15 {
                // aim v0 in random direction, giving more height to upwards vector and scale add to velocity
		random		v0
		v02			v02+1
		addScale	parentVelocity v0 velocity 350
		yaw			360*rand
		pitch		360*rand
		roll		360*rand
                model models/gibs/sphere.md3
                size 1
		emitter 1 + rand*0.5 {
			sink 0.9 50
			moveBounce 800 0.6
                        // wolfcam  this is called after impact and distance
			//anglesModel
			impact 50 {
                                if rand < 0.5 {
                                    t0 rand * 4
                                    if t0 < 1 {
                                        sound sound/misc/electrogib_bounce_01.ogg
                                    } else if t0 < 2 {
                                        sound sound/misc/electrogib_bounce_02.ogg
                                    } else if t0 < 3 {
                                        sound sound/misc/electrogib_bounce_03.ogg
                                    } else {
                                        sound sound/misc/electrogib_bounce_04.ogg
                                    }
                                }
				//size 16 + rand*32
				//shader bloodMark
				//decal alpha
			}
			distance 20 {
				clear velocity
                                shader wc/tracer
                                size 2.5 + rand * 1
                                color 0 1 0
				emitter 0.5 {
                                        red lerp
					rotate rotate + 10*lerp
					//size  8 + 10*lerp
					sprite
				}
			}
                        anglesModel  // wolfcam  moved down here
		}
      }
  }
}

// Freezetag thaw
// input: origin
//
// player/thawed {}


// General weapon effects
// flash is a 0.25 second event after a weapon is fired
// fire is a single event when weapon is fired
// impact is for weapons impact on something or explode like grenades
// trail is a single event for weapons without projectiles or a
//      constant event when projectile is in motion projectile is for
//      rendering the specific projectile

// wolfcam: the other ql weapons also available as 'nailgun', 'prox',
//   'chaingun', and 'hmg'
// wolfcam: weapons also have, in addition to /impact scripts, /impactflesh
//   scripts.  ex:  weapon/plasma/impactflesh
//   inputs for 'impactflesh' are: origin, dir, and end
//      'origin' is the impact point
//      'dir' is the reflected bounce direction from missile hits, for
//            machine gun, chain gun, shotgun, etc..  it's just a random
//            value
//      'end' is the direction of the shooter if their position is valid,
//            otherwise the length of this vector is 0
//
//   If a weapon doesn't have an impactflesh script 'weapon/common/impactFlesh'
//   will be checked.  'weapon/common/impactFlesh' would be a good place to
//   place custom hit sparks.


// input: origin, dir, team, clientnum, enemy, teammate, ineyes
//weapon/common/impactFlesh {
//        if (ineyes) {
//           return
//        }
//	  shader flareShader
//        size 5
//        color 1 0 1
//        emitter 4 {
//               //origin2 origin2 + lerp * 8
//               if (cgtime % 2) {
//                  color 0.8 0 1
//               } else {
//                  color 0.5 0 0
//               }
//               velocity2 200
//               moveGravity 0
//               Sprite
//        }
//
//        extraShader "powerups/battlesuit"
//        extraShaderEndTime cgtime + 2000
//}


// Bubbles in the water
// input: origin
weapon/common/bubbles {
        shader waterbubble
        clear velocity

	distance 5+rand*10 {
		size 1 + rand * 2
		random	dir
		addScale origin dir origin 10 * rand
		emitter 1+rand*0.25 {
			alphaFade 0
			origin2 origin2 + lerp * 8
			Sprite
		}
	}
}

// input: origin, team, clientnum, enemy, teammate, ineyes, dir
//   (note: if clientnum < 0  it's a non-player fired weapon.   dir,
//    and parentDir aren't available)
weapon/rocket/fire {
   soundweapon sound/weapons/rocket/rocklf1a
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/rocket/flash {
	color	1 0.75 0
	size 300 + rand*32
	light
}

// input: velocity, dir (normalized), rotate, origin, angles, axis, size
//
// no client info rocket projectiles
weapon/rocket/projectile {
	// Render the rocket
	model		"models/ammo/rocket/rocket.md3"
        // wolfcam need to do model rotation yourself
        rotate time * 1000 / 4
	dirModel

	// Sound generated by the flying rocket
	loopSound	"sound/weapons/rocket/rockfly.wav"
}

// input: origin, angles, velocity, dir (normalized), axis
//
// no client info for rocket trails
weapon/rocket/trail {
	// Set the base yellow color
	color	1 0.75 0

	size	200
	Light

	// The standard smoke puff trail thing
	color	1 1 1
	alpha	0.33
	shader	smokePuff
	rotate	360 * rand
	distance 45 {
		emitter 2 {
			alphaFade	0
			size		8 + lerp * 40
			sprite		cullNear
		}
	}
}

// input: origin, dir
//
// no client info for rocket impact
weapon/rocket/impact {
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
		Sprite
		// size will goto zero after 0.5 of the time
		size 300 * clip(2 - 2*lerp)
		Light
	}
}

weapon/bfg/fire {
   soundweapon sound/weapons/bfg/bfg_fire
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/bfg/flash {
	color	1 0.7 1
	size 300 + rand*32
	light
}

// input: velocity, dir (normalized), rotate, origin, angles, axis, size
//
// no client info
weapon/bfg/projectile {
	// ze model
	model		models/weaphits/bfg.md3
	dirModel
	// ze flight sound
	loopSound	"sound/weapons/rocket/rockfly.wav"
}

// input: origin, angles, velocity, dir (normalized), axis
//
// no client info
weapon/bfg/trail {

}

// input: origin, dir
//
// no client info
weapon/bfg/impact {
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
}

weapon/grenade/fire {
   soundweapon sound/weapons/grenade/grenlf1a
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/grenade/flash {
	color	1 0.7 0
	size 300 + rand*32
	light
}

// input: velocity, dir (normalized), rotate, origin, angles, axis, size,
//        team, clientnum, enemy, teammate, ineyes
weapon/grenade/projectile {
	// Render the grenade model
	model		"models/ammo/grenade1.md3"
        // wolfcam need to do model rotation yourself
        if velocity {
           rotate time * 1000 / 4
        }
	anglesModel
}

// input: origin, angles, velocity, dir (normalized), axis,
//        team, clientnum, enemy, teammate, ineyes
weapon/grenade/trail {
	// The standard smoke puff trail thing
	alpha	0.33
	shader	smokePuff
	angle	360 * rand
	// Emit a sprite every 50 milliseconds
	interval  0.05 {
		emitter 0.700 {
			alphaFade	0
			size		8 + lerp * 32
			sprite		cullNear
		}
	}
}

// input: origin, dir, surfacetype
//
// no client info
weapon/grenade/impact {
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
}

weapon/plasma/fire {
   soundweapon sound/weapons/plasma/hyprbf1a
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/plasma/flash {
	color	0.6 0.6 1
	size 300 + rand*32
	light
}

// input: velocity, dir (normalized), rotate, origin, angles, axis, size
//
// no client info
weapon/plasma/projectile {
	// The sprite for the plasma
	size 16
        rotate rand * 360
	shader	sprites/plasma1
	sprite
	// Plasma flying sound
	loopSound	"sound/weapons/plasma/lasfly.wav"
}

// input: origin, angles, velocity, dir (normalized), axis
//
// no client info
weapon/plasma/trail {

}

// input: origin, dir, surfacetype
//
// no client info
weapon/plasma/impact {
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
}

weapon/rail/fire {
   soundweapon sound/weapons/railgun/railgf1a
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/rail/flash {
	color	1 0.6 0
	size 300 + rand*32
	light
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/rail/impact {
	vibrate 50
	sound	sound/weapons/plasma/plasmx1a.wav

	pushparent color2
	pop color

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
}

// input: origin, parentOrigin, end, dir, parentDir, color1, color2, team,
//        clientnum, enemy, teammate, ineyes
weapon/rail/trail {
	// Color1 gets set before calling
	// The beam
	size  r_railCoreWidth*0.5
	shader railCore
	emitter cg_railTrailTime * 0.001 {
		colorFade 0
		Beam
	}
	shader railDisc
	// color2 is stored in the parent input structure, retrieve it like this
	pushparent color2
	pop color
	//if cg_oldrail {
        if cg_railQL {  // wolfcam: 'cg_oldrail' doesn't exist
		// Do the rail rings -- inside the rail beam
		size r_railWidth*0.5
		// Rings take their stepsize from the width variable
		width r_railSegmentLength
		emitter cg_railTrailTime * 0.001 {
			colorFade 0.1
			Rings
		}
	}

        if cg_railRings {  // wolfcam: 'cg_oldrail' doesn't exist
		// Do a spiral around the rail direction
		// Get length in t0
		t0 dir
		normalize dir
		// Create a perpendicular vector to the beam
		perpendicular dir v0
		// Radius around center
		scale v0 v0 5
		size 1.1
		// Set the rotate around angle to start value
		t1 rand*360
		// Go through length of beam in repeat steps
		repeat ( t0 / 5 ) {
			rotatearound v0 dir v1 t1
			// Slightly increase beyond regular 10 it so they don't all line up
			t1 t1 + 10.1
			addScale v1 dir origin loop * t0
			add parentOrigin origin origin
			emitter 0.6 + loop * t1 * 0.0005 {
				//5 radius + 6
				addScale origin v1 origin lerp * 1.2
				colorFade 0.7
				Sprite
			}
		}
	}
}

weapon/shotgun/fire {
   soundweapon sound/weapons/shotgun/sshotf1b
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/shotgun/flash {
	color	1 1 0
	size 300 + rand*32
	light
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/shotgun/impact {
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
}

// input: origin, end, dir, clientnum, enemy, teammate, ineyes
weapon/shotgun/trail {
        // rtcw / et style moving tracers

        if (rand > cg_tracerChance) {  // cg_tracerChance.value == 0.4
            return
        }

        // move randomly forward a bit
        // le->startTime = cg.time - (cg.frametime ? (rand()%cg.frametime)/2 : 0);
        normalize dir
        addscale origin dir origin (rand * 160)

        sub end origin v0
        t0 v0   // t0 == distance
        if (t0 < (2.0 * cg_tracerLength)) {  // cg_tracerLength.value == 160.0 rtcw
            return
        }

        // subtract the length of the tracer from the end point so that
        // it doesn't go through the end point
        addscale end dir end -cg_tracerLength
        sub end origin v1
        t1 v1  // t1 == adjusted distance


        // cg_tracerSpeed.value == 4500.0  rtcw, not in q3
        scale dir velocity 4500.0
        shader gfx/misc/tracer
        size cg_tracerWidth  // 0.8 rtcw
        copy dir v3
        copy origin v5
        emitter (t1 / 4500.0) {  // cg_tracerSpeed == 4500.0 rtcw
            // v5 is original 'origin'
            // v3 is old 'dir', 'beam' uses 'dir' to determine end point
            addscale v5 v3 origin (t1 * lerp)

            addscale origin v3 v4 cg_tracerLength
            sub v4 origin dir
            beam
        }

        addscale v5 dir origin (cg_tracerLength / 2.0)
        sound sound/weapons/machinegun/buletby1.wav
}

weapon/machinegun/fire {
    soundlistweapon {
        sound/weapons/machinegun/machgf1b
        sound/weapons/machinegun/machgf2b
        sound/weapons/machinegun/machgf3b
        sound/weapons/machinegun/machgf4b
    }
}

// input: origin, dir, team, clientnum, enemy, teammate, ineyes, surfacetype
weapon/machinegun/impact {
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
}

// input: origin, end, dir, clientnum, enemy, teammate, ineyes
weapon/machinegun/trail {
        // rtcw / et style moving tracers

        if (rand > cg_tracerChance) {  // cg_tracerChance.value == 0.4
            return
        }

        // move randomly forward a bit
        // le->startTime = cg.time - (cg.frametime ? (rand()%cg.frametime)/2 : 0);
        normalize dir
        addscale origin dir origin (rand * 160)

        sub end origin v0
        t0 v0   // t0 == distance
        if (t0 < (2.0 * cg_tracerLength)) {  // cg_tracerLength.value == 160.0 rtcw
            return
        }

        // subtract the length of the tracer from the end point so that
        // it doesn't go through the end point
        addscale end dir end -cg_tracerLength
        sub end origin v1
        t1 v1  // t1 == adjusted distance


        // cg_tracerSpeed.value == 4500.0  rtcw, not in q3
        scale dir velocity 4500.0
        shader gfx/misc/tracer
        size cg_tracerWidth  // 0.8 rtcw
        copy dir v3
        copy origin v5
        emitter (t1 / 4500.0) {  // cg_tracerSpeed == 4500.0 rtcw
            // v5 is original 'origin'
            // v3 is old 'dir', 'beam' uses 'dir' to determine end point
            addscale v5 v3 origin (t1 * lerp)

            addscale origin v3 v4 cg_tracerLength
            sub v4 origin dir
            beam
        }

        addscale v5 dir origin (cg_tracerLength / 2.0)
        sound sound/weapons/machinegun/buletby1.wav
}

weapon/lightning/fire {
    soundweapon sound/weapons/lightning/lg_fire
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/lightning/flash {
	color	0.6 0.6 1
	size 300 + rand*32
	light
}


// input: origin, dir, end (wolfcam), team, clientnum, enemy, teammate, ineyes
weapon/lightning/trail {
	//shader "lightningBoltNew"
        // wolfcam ql uses lightningBolt[1 - 7]
        shader "lightningBolt1"
	// Standard q3 beam size
	size 16
	// The beam is repeated every 45 degrees
	angle 45
	// Draw the beam between origin in direction of
        if ineyes {
        	beam depthhack
        } else {
                beam
        }
	// Length of beam in t0
	t0 dir
	// Check if we need to render the impact flash effect thing
	// That means we end before the maximum lightning range
	if ( t0 < 768 ) {
		// Clear the previous shader so it uses the one in the model
		shaderClear
		model "models/weaphits/crackle.md3"
		size 0.6  // 1
		// Slightly before the endpoint
		addScale origin dir origin ( (t0 - 16 ) / t0 )
		// Just a random model in it's angles
		angles0 rand * 360
		angles1 rand * 360
		angles2 rand * 360
		anglesModel
	}
}

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
	decal

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
		emitter "0.6 + rand*0.3" {
                        size (t0 * (1.0 - lerp))
			//moveGravity 300
			moveBounce 300 0.7
			colorFade 0.3  // 0.7
			Sprite
		}
	}
}

weapon/chaingun/fire {
    soundlistweapon {
        sound/weapons/vulcan/vulcanf1b
        sound/weapons/vulcan/vulcanf2b
        sound/weapons/vulcan/vulcanf3b
        sound/weapons/vulcan/vulcanf4b
    }
}

// input: origin, end, dir, clientnum, enemy, teammate, ineyes
weapon/chaingun/trail {
        // rtcw / et style moving tracers

        if (rand > cg_tracerChance) {  // cg_tracerChance.value == 0.4
            return
        }

        // move randomly forward a bit
        // le->startTime = cg.time - (cg.frametime ? (rand()%cg.frametime)/2 : 0);
        normalize dir
        addscale origin dir origin (rand * 160)

        sub end origin v0
        t0 v0   // t0 == distance
        if (t0 < (2.0 * cg_tracerLength)) {  // cg_tracerLength.value == 160.0 rtcw
            return
        }

        // subtract the length of the tracer from the end point so that
        // it doesn't go through the end point
        addscale end dir end -cg_tracerLength
        sub end origin v1
        t1 v1  // t1 == adjusted distance


        // cg_tracerSpeed.value == 4500.0  rtcw, not in q3
        scale dir velocity 4500.0
        shader gfx/misc/tracer
        size cg_tracerWidth  // 0.8 rtcw
        copy dir v3
        copy origin v5
        emitter (t1 / 4500.0) {  // cg_tracerSpeed == 4500.0 rtcw
            // v5 is original 'origin'
            // v3 is old 'dir', 'beam' uses 'dir' to determine end point
            addscale v5 v3 origin (t1 * lerp)

            addscale origin v3 v4 cg_tracerLength
            sub v4 origin dir
            beam
        }

        addscale v5 dir origin (cg_tracerLength / 2.0)
        sound sound/weapons/machinegun/buletby1.wav
}

weapon/gauntlet/fire {
    soundweapon sound/weapons/melee/fstatck
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/gauntlet/flash {
	color	0.6 0.6 1
	size 300 + rand*32
	light
}

// input: origin, team, clientnum, enemy, teammate, ineyes
weapon/grapple/flash {
	color	1 0.6 0
	size 300 + rand*32
	light
}


// Render a cheap red beam for the grapple.
// input: origin, angles, velocity, dir (normalized), axis
weapon/grapple/trail {
	size 10
	//shader mme/red
        shader flareShader
        color 1 0 0
	rotate 60
	beam
}

// Just use a rocket as a standard grapple model
// input: velocity, dir (normalized), rotate, origin, angles, axis, size
weapon/grapple/projectile {
	//Render a rocket as a grapple?
	model		"models/ammo/rocket/rocket.md3"
	dirModel
}

// wolfcam, not in q3mme
// input: origin
common/jumpPad {
    sound sound/world/jumppad
    color 1 1 1
    alpha 0.33
    shader smokePuff
    rotate 360 * rand
    emitter 1 {
        alphaFade 0
        size 32 * (1 - lerp)
        sprite cullNear
    }
}

// wolfcam, not in q3mme
// input: origin, clientnum
//
// clientnum is the person who hit the headshot
//common/headShot {
//
//}

// example for runfx
// fx version of '/cvarinterp cg_fov 0 110 10'
changeFov {
    t0  cg_fov
    emitterf 10 {
        if lerp >= 1.0 {  // last call cleanup, set cg_fov to orig value
            command cg_fov %t0
            return  // all done
        }
        //echo lerp
        t1 t0 * lerp
        command cg_fov %t1
    }
}
