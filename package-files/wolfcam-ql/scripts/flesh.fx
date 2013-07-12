// input: origin, dir, team, clientnum, enemy, teammate, ineyes

// "heatmap"
//  cg_fxFile "scripts/flesh.fx"

weapon/common/impactFlesh {
        normalize end
        //addscale origin dir origin 1000
	scale end end 100
	copy end dir

        shader railCore
        size 10  //cg_tracerWidth  // 0.8 rtcw
        emitter (10) {  // cg_tracerSpeed == 4500.0 rtcw
               beam
        }


      // no splash damage though :(

      if ineyes {  //teammate {  // for duel, avoid oppoenent's impacts  
            shader flareShader
      	    size 5
	    color 1 1 0

	    origin2 origin2 + 100

	    //origin0 origin0 + rand * 25
	    //origin1 origin1 + rand * 25
	    //origin2 origin2 + rand * 25
	    clear velocity

	    emitter 3600 {
               moveGravity 0
               Sprite depthhack
            }
      }
}

