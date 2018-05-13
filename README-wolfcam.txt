------------------------------------------------------------------------
Introduction:

WolfcamQL is a quakelive/quake3 demo player with some hopefully helpful options for demo viewing and movie making:

* demo pause/rewind/fast forward
* demo viewing without needing an internet connection
* viewing demos from other player's point of view or in freecam mode
* compatibility with all the client features that have been added to quakelive on top of the original quake3
* adjust player and projectile positions to match more closely what occurred on the demo taker's screen (demo treated as what the 'client saw' not what the 'server saw' and will also compensate for demo taker's ping)
* camera system
* q3mme style scripting to customize graphics and effects
* raise/remove limits for video rendering and also options like motion blur
* other stuff...  :p


enjoy!

-------------------------------------------------------------------------------
Installation:

Unzip wolfcamql*.zip anywhere you would like.  You do not have to, and it's not recommended to, install into quakelive's directory.

Copy the quakelive paks into baseq3.

To auto-download steam workshop maps for demo playback you should install steamcmd from https://developer.valvesoftware.com/wiki/SteamCMD and set its location in the fs_steamcmd variable.

  Ex:  set fs_steamcmd "C:\Program Files\steamcmd\steamcmd.exe"

If you want to use the same maps that an older demo was recorded with you can copy over files that start with qz (ex: qzteam1.pk3, qzdm6.pk3) from an older quakelive baseq3/ into wolfcam's baseq3/ .  Otherwise, it will use the new maps with old demos.

------------------------------------------------------------------------

If you want to use steam workshop gibs you need to copy workshop_gibs.pk3 into wolfcam-ql/.

Alternatively, if you'd like to have the original quake3 gibs and blood, you need to copy the following files from quake3's pak0.pk3:

First copy:

gfx/damage/blood_spurt.tga
gfx/damage/blood_stain.tga

from quake3 to:

wolfcamql/wolfcam-ql/gfx/damageq3/blood_spurt.tga
wolfcamql/wolfcam-ql/gfx/damageq3/blood_stain.tga

* Notice that the new directory is called damageq3.

Now copy:

models/gibs/gibs.jpg
models/gibs/leg.md3
models/gibs/skull.md3
models/gibs/abdomen.md3
models/gibs/chest.md3
models/gibs/intestine.md3
models/gibs/brain.md3
models/gibs/foot.md3
models/gibs/forearm.md3
models/gibs/arm.md3
models/gibs/fist.md3

from quake3 to:

wolfcamql/wolfcam-ql/models/gibsq3/gibs.jpg
wolfcamql/wolfcam-ql/models/gibsq3/leg.md3
wolfcamql/wolfcam-ql/models/gibsq3/skull.md3
wolfcamql/wolfcam-ql/models/gibsq3/abdomen.md3
wolfcamql/wolfcam-ql/models/gibsq3/chest.md3
wolfcamql/wolfcam-ql/models/gibsq3/intestine.md3
wolfcamql/wolfcam-ql/models/gibsq3/brain.md3
wolfcamql/wolfcam-ql/models/gibsq3/foot.md3
wolfcamql/wolfcam-ql/models/gibsq3/forearm.md3
wolfcamql/wolfcam-ql/models/gibsq3/arm.md3
wolfcamql/wolfcam-ql/models/gibsq3/fist.md3

* Notice that the new directory is called gibsq3

Now copy:

sound/player/gibimp1.wav
sound/player/gibimp2.wav
sound/player/gibimp3.wav
sound/player/gibsplt1.wav

from quake3 to:

and then place in:

wolfcamql/wolfcam-ql/sound/player/gibimp1.wav
wolfcamql/wolfcam-ql/sound/player/gibimp2.wav
wolfcamql/wolfcam-ql/sound/player/gibimp3.wav
wolfcamql/wolfcam-ql/sound/player/gibsplt1.wav



Use cl_useq3gibs 1 to enable, and cl_useq3gibs 0 to use to ql gibs.

When gibs are enabled quake3 gibs are checked first and if they aren't found it falls back to workshop gibs.

------------------------------------------------------------------------

q3 style player bleeding when they are hit:

Enabled when workshop_gibs.pk3 is copied into wolfcam-ql/ or when these are copied from quake3:

  models/weaphits/blood201.tga
  models/weaphits/blood202.tga
  models/weaphits/blood203.tga
  models/weaphits/blood204.tga
  models/weaphits/blood205.tga

into wolfcamql/wolfcam-ql/gfx/damageq3

  wolfcamql/wolfcam-ql/gfx/damageq3/blood201.tga
  wolfcamql/wolfcam-ql/gfx/damageq3/blood202.tga
  wolfcamql/wolfcam-ql/gfx/damageq3/blood203.tga
  wolfcamql/wolfcam-ql/gfx/damageq3/blood204.tga
  wolfcamql/wolfcam-ql/gfx/damageq3/blood205.tga

If these files aren't present a substitute blood shader will be used that
uses the blood*.tga images from wolfcamql/wolfcam-ql/gfx/wc.  The shader name
is wc/bloodExplostionAlt.

The bleeding effect can be turned on/off with com_blood cvar and the time
can be controlled with cg_bleedTime.

-------------------------------------------------------------------------

* view demo from other player's point of view.

In the console type:

/players

You'll get a list of players and then use /follow [number]  to watch them.

Use /follow -1  to return to the demo taker's point of view.

One of the limitations is that they won't always be present in the demo, they need to be close enough to the demo taker.

* /follow killer
  /follow victim

   Will look ahead in the demo and have those povs ready.  "wolfcam_drawFollowing 2" (the default) will highlight their names.

* wolfcam_switchMode 1 (switch back to demo taker if pov not available) 2 (try selected pov, then killer/victim, then demo taker pov)  0 (try selected pov, try closest opponent, demo pov)

* wolfcam  wolfcam_hoverTime "2000"

When you are following other players and they die don't automatically switch to a different pov, wait this amount of time in milliseconds before switching.

* wolfcam_fixedViewAngles "0"

Test/debug cvar.  It'll only update the viewangles of the /follow pov this many milliseconds.


-----------------------------------------------------------------
Matching colors and lighting with quakelive:

In order to match colors and lighting post processing isn't needed, with the exception of bloom.

Quakelive's r_enablePostProcessing and r_enableColorCorrection sort of correspond to the old q3 cvar r_ignorehwgamma.

There's a lot of confusion about what's involved in quakelive's post processing since there isn't much documentation about what the cvars do.

Here's my best take/guess on it:

In quake3 if you want to use r_gamma and r_overbrightbits you need to enable hardware gamma (r_ignorehwgamma 0).  The problem is that setting gamma like that changes the color/brightness of your entire desktop not just the quake3 application.  In quake3 it isn't a big problem since you usually play fullscreen and don't really care about messed up colors in the desktop underneath.  Quakelive was designed to be run in a browser so they wanted to avoid changing gamma for the whole screen and wanted to restrict it to just the quakelive window.  The solution was r_enablePostProcessing, which at the price of a little performance drop (an extra rendering step), enabled gamma (r_gamma + r_overbrightbits) just for the quakelive window.  At that point in quakelive's history r_enablePostProcessing  1   corresponded  to r_ignorehwgamma 0  in quake3 (allow the use of r_gamma and r_overbrightbits).

Bloom was later added  and it created a problem since r_enablePostProcessing didn't just deal with gamma anymore (it controlled both bloom and gamma).  So a later quakelive patch added r_enableColorCorrection which is basically gamma again.  This allowed you to keep r_enablePostProcessing 1  if you wanted bloom and r_enableColorCorrection 0  to disable disable gamma.  r_enablePostProcessing became a generic cvar enabling different types of post processing (gamma, bloom, anything else they might want to add, etc.).  r_enableColorCorrection 1 became the new r_ignorehwgamma 0.

So basically quakelive and quake3 have the same colors and lighting which are controlled by the usual r_* cvars (r_mapOverBrightBits, r_intensity, etc..).   The problem and confusion is that two of those:  r_gamma and r_overbrightbits  are special and have to be "unlocked" in order to have their settings apply.

In quake3/ioquake3/wolfcam(with post processing disabled) you have to have "r_ignorehwgamma 0" in order to use r_gamma and r_overbrightbits.

In quakelive you need "r_enablePostProcessing 1  and r_enableColorCorrection 1" in order to use r_gamma and r_overbrightbits.

Oh, and to add to the confusion, quakelive still has the r_ignorehwgamma cvar :(    It doesn't seem to do anything though.

-----------------------------------------------------------------
It's based on ioquake3 so you have a few features available and changed:

  cl_aviFrameRate                   - the frame rate to use when capturing video
  cl_aviMotionJpeg                  - (this cvar is removed, see cl_aviCodec)

New commands
  stopvideo               - stop video capture


see README-ioquake3.txt for more stuff.

Note: it's not mentioned in the ioquake3 documentation but you need to disable openal in order to have sound recorded in the avi file.   +set s_useopenal 0

added features:

cl_aviFrameRateDivider  integer value that will force rendering at cl_aviFrameRate * blur frames * value and drop frames so that final output still has a rate equal to cl_aviFrameRate.  Values less than 2 are ignored.

cl_aviCodec  uncompressed, mjpeg, huffyuv

cl_aviAllowLargeFiles 1  to allow opendml avi files (up to about 500 gigabytes)

/video [avi, avins, tga, jpg, png, wav, split, name <file basename>]
  All files stored in video/

  ex:  /video tga wav    to dump tga screen-shots and a wav sound recording
  ex:  /video avins wav name myNewVideo     avi file with no sound and a wav file

  'name :demoname' will use the current demo filename as the video file basename

  'split' option records extra right and left videos with r_anaglyphMode settings.  See extensions to r_anaglyphMode.

  r_jpegCompressionQuality   controls jpeg compression quality
  r_pngZlibCompression  choose between high speed or higher compression size

cl_aviFetchMode GL_RGB (default switched from ioquake3 GL_RGBA default)

When you take a screen-shot you select what format you would like the video card to send back:  "red green blue", "blue green red alpha", etc..

Some video cards might be faster or slower with certain formats.

  added default bind:
     bind i "toggle cl_avifetchmode gl_rgb gl_rgba gl_bgr gl_bgra; echopopupcva
r cl_avifetchmode"

  Do a test run and try the different values to see if you can get a performance boost when rendering.  Note that there is no information in the alpha channel, so this isn't a way of getting transparency information.

Note:  png output and/or motion blur disregard this option and force GL_RGBA as the fetch mode.

cl_aviNoAudioHWOutput  don't pass audio data to sound card when recording, default is 1

cl_freezeDemoPauseVideoRecording  to pause recording of video/screen-shots while paused

cl_aviExtension  allow changing '.avi' extension to something else to avoid Windows indexing of video file

------------------------------------------------------------------

q3mme fx scripting

  /set cg_fxfile "scripts/q3mme.fx"
  /fxload

  to disable ->
  /set cg_fxfile ""
  /fxload

  /fxload <somefxfile.fx>

  These two cvars can be used to control performance for now:

  cg_fxinterval  how often in milliseconds the scripting code is run (default 25).  Lowering lowers performance.
  cg_fxratio     ratio of distance and size to determine if script code should be run for an entity (default 0.002).  Lowering lowers performance
  If an fx entity is so small and/or far away (size / distance from you) don't bother running fx code for it in order to improve performance.
  cg_fxLightningGunImpactFps  how often to run lg impact and look the same at any frame per second value (default is 125, 0 to disable)

  cg_fxScriptMinEmitter
  cg_fxScriptMinDistance
  cg_fxScriptMinInterval
     for the three above they are used to prevent values dipping below those
     thresholds

  cg_fxCompiled  enable/disable performance optimization (2 or higher enables debugging output)

  cg_vibrate
  cg_vibrateForce
  cg_vibrateTime
  cg_vibrateMaxDistance

  /fxmath 2 + 4 * 6.1      if you need a calculator or something

  note:  all the scripts need to be in one file

Some additions to q3mme fx scripting:

* shadertime to control when shader animations begin
   ex:
     weapon/rocket/projectile {
        model models/powerups/pop.md3
        shadertime cgtime / 1000.0   // sets shadertime to current time
                                     // so the shader animation never
                                     // advances
        dirModel
     }

* soundlocal, soundlistlocal, soundweapon, and soundlistweapon in addition
   to sound and souldlist to allow local (not placed inside 3d world) and
   weapon (always 'inside player head) channels.
* modulus math operator '%'.  Note: both input values are rounded to the nearest integer
* player/head/trail, player/torso/trail, player/legs/trail, player/flight
* the following are passed on to the scripting system where appropriate:  team, clientnum (less than 0 signifies non-player), enemy, teammate, ineyes, surfacetype (for impacts).  You could have something like custom rails and rockets per team or different settings for enemies and also first person view.  See wolfcam-ql/scripts/q3mme.fx for more info.

  surfacetype doesn't work that well since the maps only use it occasionally:
  surfacetype 1 == metal
              2 == wood
              3 == dust
              4 == snow

* weapon/<weapon name>/impactflesh, also weapon/common/impactflesh
      inputs:  origin, dir, and end
        'origin' is the impact point
        'dir' is the reflected bounce direction from missile hits, for
              machine gun, chain gun, shotgun, etc..  it's just a random
              value
        'end' is the direction of the shooter if their position is valid,
              otherwise the length of this vector is 0

* 'animFrame' variable added to control model animations, player/[legs|torso|head]/trail automatically set this from current player animation

* additional commands for testing/debugging:
       'return'  to exit the current script
       'continue'  to skip the current block (skip to end brace)
       'echo <var>' to print a scalar value

* 'command' can be used to send a console command.  '%' indicates a script var and the final command will have that script var's value inserted.  Use '%%' to print a single percent sign.

Ex:

        command echo the value of v00 is %v00
            -> will print "the value of v00 is 0.549834" to the console

        command echopopup %% t0 is %t0
            -> will pop up onscreen message "% t0 is 2.66"

* emitterId float to tag emitters and also, if set to a negative, number will
  mark it as low priority (first to be dropped) if the maximum entity is
  reached.

Ex:
        distance 10.0 {

             // check for first person view and tag it
             // since 1st and 3rd person gun origin is different.  A 1st
             // person effect might look bad when you switch to 3rd person.
             // You can clear it with:  /clearfx 6

             if ineyes == 1 {  // it's first person view
                emitterId 6
             }
             emitter 0.25 {
                // ...
                sprite
             }
         }

/localents
  to list fx and local entities

/clearfx [emitter id number]
  without id number clears all

* cg_fxDebugEntities  (1:  print a message when a non low priority entity
    needed to be dropped, 2:  print a dropped message even for low priority
    entities, 3:  add on screen display of number of local entities, 4:  additional debugging information, -1:  only on screen display of number of local entities)


* 'loopcount'  as an alternative to 'loop':

  repeat 6 {
    t0  loop  // will go from 0.0 -> 1.0  (0.0, 1.666, ...)
    t1  loopcount // will go from 1 to 6  (1, 2, ...)
  }

* cullDistance render token and cullDistanceValue variable to prevent rendering of entities far away.  Ex:

   emitter 2 {
      ...
      cullDistanceValue 400  // set cull value
      sprite cullDistance  // set sprite to use cullDistanceValue

* some additional trig/math functions:  acos, asin, atan, atan2, pow

  Note that atan2 and pow have the following syntax:
    atan2 val1 val2   // or the following
    pow (val1) (val2)

  The following syntax is invalid:
    pow(10, 2)  // invalid

* additional vars that can be read: 'end' (vector) (can also be written), 'cgtime' server time, 'team', 'clientnum', 'enemy', 'teammate', 'ineyes' (is it first person view), 'surfacetype', 'gametype' (see cg_gametype description in ql hud extensions)

* additional vars for reading and writing:  t0 up to t9  and v0 up to v9

* impact scripts have the reflected velocity (off surface) in 'velocity', see scripts/dirtest.fx (red stuff is the reflected velocity and blue is 'dir' or the perpendicular vector to the face plane of the struck surface)

* 'emitterf' which lets lerp go from 0.0 to greater than 1.0.  checking for lerp >= 1.0  can be used for clean up or to set final values

* 'extraShader', 'extraShaderClear', and 'extraShaderEndTime' for scripts associated with player entities.  For 'impactFlesh' scripts these refer to the victim and for other weapon scripts generally the attacker.

* /runfx and /runfxat to run arbitrary scripts and add effects independent of game events and can be used as a generic scripting system

  /runfx <fx name> [origin0] [origin1] [origin2] [dir0] [dir1] [dir2] [velocity0] [velocity1] [velocity2]
       if origin0 --> dir2 not given will uses current freecam or 1st person view settings.

  /runfxat <'now' | server time | clock time> <fx name> [origin0] [origin1] [origin2] [dir0] [dir1] [dir2] [velocity0] [velocity1] [velocity2]

  (see 'at' command for time information)

    ex:  /bind y runfx player/gibbed

    ex: /runfx changefov

    // fx version of '/cvarinterp cg_fov 0 110 10'
    changeFov {
        t0  cg_fov
        emitterf 10 {
            if lerp >= 1.0 {
                command cg_fov %t0
                return  // all done
            }
            //echo lerp
            t1 t0 * lerp
            command cg_fov %t1
        }
    }

* /runfxall <script name>  will run script against all present fx entities
  see scripts/alltest.fx :

  set cg_fxfile "scripts/alltest.fx"
  bind z "runfxall moveup"
  /devmap campgrounds
  /give all
  shoot some lg and hit 'z'

* /listfxscripts
* /printdirvector  (prints current viewangles as a dir vector)


Some notes about compatibility with q3mme and some general things:

1) q3mme lists 'cross' as a supported function but it's not implemented in q3mme.  You can use this instead:

  //cross v0 v1 v2
  v20  v11*v22 - v12*v21
  v21  v12*v20 - v10*v22
  v22  v10*v21 - v11*v20

2) In wolfcamql scripts are not byte compiled or something equivalent so they
  run much slower and you might need to be careful with low values of 'distance' and 'interval'.  Note that it won't affect how they appear when recording video.

3) 'else' is broken in q3mme.  The script will die when it reaches the end of an 'else' block.  You can work around it by doing something like this:

   t0 time * 2
   if loop < 0.5 {
      t0 time * 3
   }

   instead of:

   if loop < 0.5 {
      t0 time * 3
   } else {
     t0 time * 2
     // q3mme bug -- script will die here
   }

4) fps issue:

    'interval' and 'distance' used to be triggers, meaning:

    weapon/grenade/trail {
        //...
        interval 0.001 { // if at least this much time has passed, run the following code
        //
        }
    }

    but now, in order to be more compatible with q3mme and to reduce timescale and fps issues the total time or distance is split into chunks of the size indicated and the code gets run for each chunk. So, say you have com_maxfps 125, every time the screen gets drawn 8 milliseconds have passed (1000.0 / 125.0) and in earlier versions the code would have been run once (0.008 > 0.001), but now the code will get run 8 times. You can't use 'interval 0' or 'distance 0' any more (see your weapon/grenade/trail).

    It's generally better to use 'distance' instead of 'interval' since 'interval' will always be fps and timescale dependent. Imagine the difference between com_maxfps 90 and 30. Projectile positions will be update 3 times more with com_maxfps 90, so if you do something like a trail that just spits out a sprite at origin you'll have 3 sprites clumped together with com_maxfps 30 while com_maxfps 90 will have them spread out.

    // test trail
    // change around max fps and see how it affects trails
    weapon/rocket/trail {
        color 1 0 0
        green com_mafps / 125.0
        alpha 0.33
        shader flareShader
        rotate 360 * rand
        interval 0.012 { // 12 milliseconds
        emitter 200 {
            alphaFade 0
            size 8
            sprite
            }
        }
    }

    You can fix it be using 'distance' instead. Rockets travel at 900ups, so they will have moved 10.8 (900 * 0.012) game units:

    //interval 0.012 { // 12 milliseconds, fps and timescale dependent
    distance 10.8 { // will look the same at any fps and timescale
        // ...
    }

    Keep in mind that now that 'interval' and 'distance' blocks can be run multiple times it's now very easy to shoot yourself in the foot. You need to be careful using small values. Using fps 125 the screen is updated every 8 milliseconds so a rocket will move 7.2 (900 * 0.008) units every time the screen is drawn. If you do something like 'distance 0.001 {' the program will freeze as it tries to run that code 7200 times (7.2 / 0.001).

    One other issue is using small 'emitter' times as well. If they are too small they might not even be drawn and there's no way around them not being fps and timescale dependent (other than setting a worst case scenario minimum value: cg_fxScriptMinEmitter). Thirty frames per second is common for movies, so that means that emitted entities with lifetimes of less than 33.333_ (1000.0 / 30) milliseconds might not be drawn.

    If you have a script that uses small 'emitter' values and you find that it looks well at some low timescale but not at higher ones, you can try to simulate the lower timescale:

    //-------------------------------------------------------------
    // original version, looks bad at timescale 1.0 and good at
    // timescale 0.3

    distance rand * 10 {
             ...
             emitter 0.008 * rand {
                     ...
             }
    }

    //-----------------------------------------------------------

    change to this:

    // script looks good at timescale 0.3 but not at timescale 1.0
    // try to simulate 0.3
    t0 timescale
    t1 1  // random value to check against
    if t0 > 0.3 {
       t1 1.0 / (t0 / 0.3)
    }

    distance rand * 10 {
             if (rand < t1) {  // simulate timescale 0.3
                  ...
                  emitter 0.008 * rand {
                        ...
                  }
             }
    }


5) fx scripting (both q3mme and wolfcamql): don't use multiple 'distance' or 'interval' blocks in scripts. It doesn't break things but wont work as expected:

   weapon/rocket/trail {
       distance 45 {
       // stuff
   }

   distance 45 { // this wont get run because the last 'distance' block has updated the stored settings about how much the rocket moved
       // more stuff
   }

6) FIXME -- wolfcamql  c-style comments:  /* sldkfj */  not supported yet

7) In wolfcamql weapon/*/[fire|trail] uses weapon muzzle point for 'origin' as opposed to player origin in q3mme.  The real player origin is passed as 'parentOrigin' for weapon/*/fire scripts.  If clientNum < 0  it's a non-player fired
weapon (ex: map grenades in silentnight) and dir, parentOrigin, and parentDir
aren't available.

8) The maximum number of entities for wolfcamql is a little over 16000.

9) The maximum number of dynamic lights for both wolfcamql and q3mme is 32.
  You should restrict the use of 'Light' to flash effects, explosions, and
  possibly rocket projectiles.

10) wolfcam incompatibility with q3mme fx:  In wolfcam scripting for an emitted entity stops once it has been added, so 'impact', 'distance', and 'interval' scripts need to come before 'sprite', 'model', and other rendering directives.
    Ex:

         emitter 10 {
            ...
            sprite  // in wolfcam scripting stops here
            distance 50 {
               // in wolfcam this is never run, it will run in q3mme
            }
            //sprite  // if it's added here in wolfcam it will now run
         }

    This is because in q3mme the the 'emitter' is the entity and 'sprite' is a directive to draw itself.  In wolfcam the 'sprite' is an entity separate from 'emitter'.  Ex:

         emitter 10 {
            ...
            origin2 origin2 + 100
            sprite
            ...
            origin2 origin2 + 100
            sprite
            ...
            origin2 origin2 + 100
            sprite
            ...
         }

    In q3mme this is a 'particle' that will draw itself 3 times in 3 different places at the same time, in wolfcam this creates 3 different and independent 'sprite' particles.

---------------------------------------------------------------

client options:

* You can use really slow timescales (up to 1 millionth of a second:  /timescale 0.0000001), for most people you usually couldn't go below 0.1

* com_timescalesafe default 1  high timescales will skip demo snapshots and will break things like camera paths that are synced to server times.

  In quake3 with really high timescale values, information from the demo starts to get discarded.  You can lose information about frags, chat, etc.  Having this set to 1 ensures that all the information (snapshots) in the demo get processed.


* 'fixedtime' cvar can use values less than 1.0

----------------------------------------------------------------

* /clientoverride similar to q3mme

  /clientoverride <client number or 'red', 'blue', 'enemy', 'mates', 'us', 'all', 'clear'> <key name> <key value> ... with additional key and value pairs as needed

  example:  clientoverride 3 model ranger hmodel bones

  'clear' option can also be followed by 'red', 'blue', 'enemy', 'mates', 'us', 'all', or a client number.  The default is 'all'.

  example:  clientoverride clear mates


  To see what can be changed type /configstrings in the console and scroll up to the player list:

  529: n\WolfPlayer\t\1\model\sarge\hmodel\sarge\c1\5\c2\26\hc\100\w\0\l\0\tt\0\tl\0\rp\0\p\0\so\0\pq\0\cn\\su\0

  n      name
  t      team
  model  model
  hmodel head model
  c1     color1 for rail
  c2     color2 for rail
  hc     handicap
  w      wins  (seen in the old scoreboard for duel)
  l      losses  (seen in the old scoreboard for duel)
  cn     clan tag
  xcn    clan name
  tt     team task (old team arena stuff)
  tl     team leader (old team arena stuff)
  rp     ready up status?  (qldt)
  p      ?
  so     spectator only?
  pq     player queue
  su     subscriber (whether the player has a pro or premium account)
  c      country acronym

  The rest I don't remember.

  Additionally you can use 'headskin', 'torsoskin', 'legsskin', 'headcolor', 'torsocolor', 'legscolor', 'modelscale', 'legsmodelscale', 'torsomodelscale', 'headmodelscale', 'headoffset', 'modelautoscale' (match this height)

      note: skins use model names
        ex:  /clientoverride 3 hmodel biker headskin xaero/blue

  cvar cg_clientOverrideIgnoreTeamSettings  setting to 1 will ignore will ignore your settings for things like cg_enemyModel and use the value you set with /clientoverride

----------------------------------------------------------------

/changeconfigstring [force | clear | list] <string number> <new string>
  ex:  changeconfigstring 3 "Furious Heights"   // changes the map name
  ex:  changeconfigstring force 659 ^6noone
  ex:  changeconfigstring clear 1
  ex:  changeconfigstring clear all
  ex:  changeconfigstring list
   3 is map name, 659 is player in first place

  'force' option will ignore any changes that occur in the demo and keeps the
   value you had set

/cconfigstrings
  same as /configstrings but shows strings with overridden values

----------------------------------------------------------------

QuakeLive scripted hud extensions

* you can switch fonts in hud configs, use "font" keyword

* libfreetype2 font support:
  Fonts can be set in your hud config and need to have a .ttf extension.  Try to avoid putting them in wolfcam-ql/fonts.  Ex:  copy  arial.ttf into wolfcam-ql/myfonts/  and then in your hud config:

    //      font "fonts/smallfont" 12
    //      smallFont "fonts/smallfont" 24
    //      bigFont "fonts/smallfont" 48

        font "myfonts/arial.ttf" 12
        smallFont "myfonts/arial.ttf" 24
        bigFont "myfonts/arial.ttf" 48

q3fonts available as "q3tiny", "q3small", "q3big", "q3giant"
qlfont available as "fontimage"

Fonts can also be used for hud config elements:

        itemDef {
        name "a"
        rect 16 5 63 12
        visible 1
        textstyle 3
        decoration
        textscale .45
        forecolor 1 1 1 1
        ownerdraw CG_PLAYER_AMMO_VALUE
        addColorRange 6 999 1 .75 0 1
        //font "fontsextra/times.ttf" 48
        font "q3small" 48
        }


* /loadhud ui/somehud.cfg  to avoid having to set cg_hudfiles

* cg_hudRedTeamColor  cg_hudBlueTeamColor  cg_hudNoTeamColor  for use with hud element CG_TEAM_COLOR

* cg_hudForceRedTeamClanTag and cg_hudForceBlueTeamClanTag  (edit the team names in score-box)

* allow absolute values for coordinates (see cg_wideScreen)

* hud variables

  Anywhere in a hud menu you can use 'setvar @varname (0.8)' to store a value and/or create a variable.  You can then use the variable anywhere you would normally use a decimal or integer number.  Note:  parenthesis, '(' and ')', are required.

  ex:

    itemDef {
       ...
       setvar @tscale (0.35)
       textscale @tscale
       ...
    }

* use math functions and variables to change user created variable values
  (same as q3mme fx scripting)

    + - / * %
    > < != =        (to test equality)
    & |             (logical AND OR)
    sqrt, ceil, floor, sin, cos, wave, clip, acos, asin, atan, atan2, pow
    rand            (create a random value between 0.0 and 1.0)
    crand           (create a random value between -1.0 and 1.0)
    realtime        (system time in milliseconds)
    gametime        (game/server time in milliseconds)
    pi              (pi constant)
    ownerdrawvalue

  ex:
     setvar @xscale ((@xscale / 2.0) * rand)
     setvar @players (ownerdrawvalue CG_PLAYER_COUNTS)

  Note that atan2 and pow have the following syntax:
    atan2 val1 val2   // or the following
    pow (val1) (val2)

  The following syntax is invalid:
    pow(10, 2)  // invalid


  Math processing falls back to trying a cvar if an unknown keyword is found.
   ex:
      setvar @var (r_fullscreen + @otherVar)

   If the cvar doesn't exist an error occurs.

* if/else code blocks within itemDefs  (Note: not allowed in higher scopes like menuDef, also menu needs to be visible)

  ex:
   menuDef_t {
    visible 1

    itemDef {
        ...
        if (@xscale > 0.35) {
           forecolor 1 0 0 1
        } else {
           forecolor 0 1 0 1
        }
        ...
    }
   }

* itemDefs can have an optional 'run' script that gets run every screen draw and allows you to change things dynamically

  ex:

    // silly example that randomly changes the level timer size
    itemDef {
        name "Timer"
        textalign ITEM_ALIGN_LEFT
        style 1
        rect 0 0 50 12
        visible 1
        textstyle 3
        decoration
        forecolor 1 0.64 0.04 .7
        ownerdraw CG_LEVELTIMER

        run {
            setvar @xscale (0.7 * rand)
            textscale @xscale
        }
    }

* itemDef command 'cvarSet' to set a cvar to specified string value
     ex:  cvarSet tmpCvar "this is a tmpCvar"

* itemdef command 'textExt' to embed script variables and cvar strings into text:
    textExt "some text %FORMAT_STRING{SCRIPTVAR_NAME}"
      ex:
        setvar @numPlayers (ownerdrawvalue CG_PLAYER_COUNTS)
        textExt "Number of players: %d{@numPlayers}"

      ex:
        setvar @speed (ownerdrawvalue CG_SPEEDOMETER)
        if (@speed > 320) {
          textExt "^1%.2f{@speed} ^7ups"
        } else {
          textExt "^2%.2f{@speed} ^7ups"
        }

      ex:
        cvarSet myTmpCvar "testing ^3text ^7input"
        textExt "show this text: %s{myTmpCvar}"

    use '%%' to print a a single '%'
    only 'd', 'f', and 'x' accepted as format conversions and a hud script variable is used to get the value
    's' will check the string value of a cvar

* itemdef command 'textReset' to remove text entry in menu

* itemdef and menudef command 'backgroundReset' to remove background image

* printval for debugging

  ex:  printval (@rectWidth)

* itemDef 'visible' can be toggled which might cause incompatibility.

  ex:

    itemDef {
      ...
      ownerdraw CG_1STPLACE_PLYR_MODEL
      visible 1  // in quake live this will make it visible even if you set to '0' later on
      visible 0  // ignored in quake live but in wolfcam will disable visibility
      ...
    }

* additional ownerdraws:  include ui/wcmenudef.h to use them

  WCG_GAMESTATUS

    ex:

       // quake3 osp style warmup message
       setvar @gameStatus (ownerdrawvalue WCG_GAME_STATUS)

       if (@gameStatus = W_STATUS_WARMUP) {
         font "q3tiny" 48

         setvar @fadeTime (900)
         setvar @alpha (realtime % (@fadeTime * 2))
         if (@alpha > @fadeTime) {
            setvar @alpha (1.0 - ((@alpha - @fadeTime) / @fadeTime))
         } else {
            setvar @alpha (@alpha / @fadeTime)
         }

         textStyle ITEM_TEXTSTYLE_SHADOWEDMORE
         forecolor 1 0 0 @alpha
         textExt "Waiting for Players"
       }


  WCG_WEAPON_SELECTED, WCG_WEAPON_SELECT_TIME, WCG_WEAPON_HAVE_MACHINEGUN, WCG_WEAPON_AMMO_MACHINEGUN, and others to allow creation of custom weapon bars.  See wolfcam-ql/ui/wcweaponbar.menu for an example.  You can test it in game with '/loadmenu ui/wcweaponbar.menu'.

  WCG_KILL_COUNT  number of kills since last death or reset ("killcountreset"
                  command)

  WCG_PLAYER_KEY_PRESS_[FORWARD|BACK|RIGHT|LEFT|FIRE|JUMP|CROUCH]

* cg_gametype variable checking corresponds to quake with additional support for quake3 and cpma gametypes:

  0   ffa
  1   duel
  2   race
  3   team death match
  4   clan arena
  5   ctf
  6   one flag ctf
  7   obelisk
  8   harvester
  9   freezetag
  10  domination
  11  capture strike (ctfs), attack / defend
  12  red rover
  13  cpma 'not team fortress'
  14  cpma 2v2
  15  cpma hoony mode
  16  quake3 single player

* ownerdraws removed in Aug 27, 2014 quakelive update can still be referenced by including ui/wcmenudef.h

* /loadmenu [menu file]

  To test / debug individual menu files.

----------------------------------------------------------------

* cg_quadFireSound  to enable/disable the extra sound effect when a player with quad power-up fires their weapon

* cg_animationsIgnoreTimescale  animations will play in real-time regardless of what timescale setting is.  Default is 0.

* cg_animationsRate  you can speed up (> 1.0) or slow down animations (< 1.0).  Default is 1.0 .

* cg_markTime and cg_markFadeTime  to control wall/floor impact marks

* s_useTimescale  adjust audio pitch according to timescale
* s_forceScale  force audio to scale pitch
* s_showMiss  debug cvar to show skipped sounds (1:  skipped because of max repeat rate, 2: also show if it was skipped because of max number of sounds being played, >2:  show if sound channel couldn't be allocated)
* s_show  >2 will only output when a sound is played
* s_maxSoundRepeatTime  skip playing a sound if it is repeated this often in milliseconds  (default is 0 to match quakelive, q3 is 50)  Noticeable with lg hit beeps
* s_maxSoundInstances  (96: default similar to quake live, -1: q3 code which limits either 4 or 8 sounds attached to an entity -- with the bug of the 3d world counting as an entity)
* s_qlAttenuate  (0: q3 style -- use smaller distance for sound volume calculation and always play at full volume sounds within 80 game units, 1: default calculate sound volumes like quake live)
* s_debugMissingSounds

* cg_audioAnnouncer  (0:  disable audio announcer, 1:  Vadrigar (Classic), 2:  Daemia (Female), 3:  new ql announcer)

* cg_ignoreServerPlaySound  disable/enable server 'playsound' command.  Used with 'crash' tutorial and minqlx custom sounds.

------------------------------------------------------------

Audio messages not dependent on cg_draw2d, use:  cg_audioAnnouncerRewards, cg_audioAnnouncerRound  ("round begins in ...  fight!", "blue wins the round", ...), cg_audioAnnouncerWarmup ("3 2 1 fight!", ...) , cg_audioAnnouncerVote, cg_audioAnnouncerTeamVote, cg_audioAnnouncerFlagStatus, cg_audioAnnouncerLead  ("taken the lead", "tied", ...), cg_audioAnnouncerTimeLimit  (time-limit warnings), cg_audioAnnouncerFragLimit  (frag-limit warnings), cg_audioAnnouncerWin, cg_audioAnnouncerScore ("red scores", ...), cg_audioAnnouncerLastStanding, cg_audioAnnouncerDominationPoint, cg_audioAnnouncerPowerup (powerup pickup)

* cg_audioAnnouncerRoundReward  to enable/disable end of round 'perfect' or 'denied' announcements

* cg_audioAnnouncerRewardsFirst  enable first reward having a unique voice over

* cg_attackDefendVoiceStyle (0: 'fight!',  1: like quake live -- 'attack/defend the flag')

* s_announcerVolume

------------------------------------------------------------

* cg_buzzerSound  end of game buzz sound (same as quakelive)

* cg_spawnArmorTime  new spawn armor shader for spawn protected players, default 500

* r_forceMap
        ex:  set r_forceMap "pro-nodm9"; demo FFA-lostworld.dm_73

* cg_customMirrorSurfaces  (when using r_forcemap will use the map's mirror information to add reflected scenes)
   test command:  /addmirrorsurface <x> <y> <z>

* q3mme motion blur:  mme_blurFrames, mme_blurType, mme_blurOverlap

* depth buffer saving:  mme_saveDepth, mme_depthRange, mme_depthFocus

  With 'cl_renderer opengl2' use 'r_useFbo 1'.

* cg_plasmaStyle same as quakelive (2: purple bubble trails)

* r_jpegCompressionQuality 90 (default)  for screen-shots, jpg video dump, and 'cl_aviCodec mjpeg'

* cg_checkForOfflineDemo 1 (default)  demos recorded using /devmap or offline bot play wont stutter

* cg_muzzleFlash same as quakelive

* auto vstr for weapons:  cg_weaponDefault, cg_weaponNone, cg_weaponGauntlet, cg_weaponMachineGun, cg_weaponShotgun, cg_weaponGrenadeLauncher, cg_weaponRocketLauncher, cg_weaponLightningGun, cg_weaponRailGun, cg_weaponPlasmaGun, cg_weaponBFG, cg_weaponGrapplingHook, cg_weaponNailGun, cg_weaponProximityLauncher, cg_weaponChainGun, cg_weaponHeavyMachineGun

  ex:  set cg_weaponRailGun "cg_fov 90; cg_drawCrosshair 1"
       set cg_weaponDefault "cg_fov 110; cg_drawCrosshair 4"

* cg_levelTimerDirection:  0, 1  same as quakelive, including bugs (always count up during overtimes regardless of the settings you have chosen), 2  count up and don't reset to 0 during overtimes  (ex:  14:53  is shown for duel 53 seconds into the second overtime), 3 countdown even during overtime and for matches with no time-limit use cg_levelTimerDefaultTimeLimit, 4 always count down and try to use the end of game time parsed from the demo -- otherwise use cg_levelTimerDefaultTimeLimit

* cg_levelTimerOvertimeReset:  0: (default) shows total overtime amount, 1: shows current overtime amount

* r_lightmapColor to tint the lightmap
  extreme example:  r_lightmapColor "0xff0000" to make all the lights red
  better example: r_lightmapcolor "0xffafaf" to turn down blue and green a bit, without completely eliminating them

  With 'cl_renderer opengl2' you need to use 'r_mergelightmaps 0'.

----------------------------------------------------------------

In game camera creation and editing
  /addcamerapoint  ('v' default)

     Adds a new camera point if none have been created yet or if the current
     time is greater than the last camera point.

     Inserts an additional camera point if current time lies between two camera
     points that are already present.

     If current time matches a camera point will change those camera point
     values to match freecam values (origin, angles, roll, etc..)

  /clearcamerapoints
  /playcamera  ('o' default)
  /stopcamera
  /loadcamera <filename>
  /savecamera <filename>

  The default location for camera files is cameras/.  Camera points are automatically saved to cameras/wolfcam-autosave any time you make a change.

  /selectcamerapoint <point 1> [point 2]
    or
  /selectcamerapoint [all, first, last, inner(don't include first and last)]

  /editcamerapoint (no args will edit currently selected) or  [next, previous]

     /editcamerapoint next (']' default)
     /editcamerapoint previous ('[' default)

  /deletecamerapoint

    will delete all the selected camera points

  /ecam

  type /ecam by itself or /ecam help  to see:

Edit all currently selected camera points
<...> are required
[...] are optional

/ecam type <spline, interp, jump, curve, splineBezier, splineCatmullRom>
/ecam fov <current, interp, fixed, pass, spline> [fov value]
/ecam command <command to be executed when cam point is hit>
/ecam numsplines <number of spline points to use for this key point (default is 40) >
/ecam angles <interp, spline, interpuseprevious, fixed, fixeduseprevious, viewpointinterp, viewpointfixed, viewpointpass, ent>
   the 'ent' option has additional parameter for the entity
   /ecam angles ent [entity number]
/ecam offset <interp, fixed, pass> [x offset] [y offset] [z offset]
/ecam roll <interp, fixed, pass, angles> [roll value]
/ecam flags [origin | angles | fov | time]
/ecam initialVelocity <origin, angles, xoffset, yoffset, zoffset, fov, roll> <v
alue, or 'reset' to reset to default fixed velocity>
/ecam finalVelocity <origin, angles, xoffset, yoffset, zoffset, fov, roll> <value, or 'reset' to reset to default fixed velocity>
/ecam rebase [origin | angles | dir | dirna | time | timen <server time>]
   edit camera times to start now or at the time given time, use current angles, origin, or direction as the new starting values
   note:  dirna updates the camera direction without altering camera angles
/ecam shifttime <milliseconds>
/ecam smooth velocity
   change camera times to have the final immediate velocity of a camera point
   match the initial immediate velocity of the next camera point
/ecam smooth avgvelocity
   change camera times to have all points match the total average velocity
   run command multiple times for better precision
* /ecam smooth origin
   match, if possible, the final immediate velocity of a camera point to the
   immediate initial velocity of the next camera point to have smooth origin
   transitions  (done by setting the appropriate overall final and initial
   velocities)
* /ecam smooth angles
  (same as '/ecam smooth origin' for view angles)
* /ecam smooth originf
  aggresive origin smoothing which will change origins and camera times in
  order to match velocities
* /ecam smooth anglesf
  aggresive angles smoothing which will change angles (but not times) in order
  to match angle velocities
* cg_cameraSmoothFactor (value between 1.0 and 2.0) used by /ecam smooth <type>
* /ecam reset
  resets all adjusted velocities
* /ecam smooth time
   change camera times so that the points have the same average velocity
* /ecam scale <speed up/down scale value>
    speed up or down the selected camera points by adjusting camera time (2.0: twice as fast, 0.5: half speed)


  /nextfield  (KP_DOWNARROW default)
  /prevfield  (KP_UPARROW default)
  /changefield (KP_ENTER default) this will toggle and or set the fields

  additional commands
  /setviewpointmark
  /gotoviewpointmark

All the *useprevious settings are used when you want to transition from following an entity (player, missile, etc.) and want to transition the viewangles without jumping.

*pass options mean to skip interpolating viewangles to the next camera point with the next camera point also having the possibility of having the pass option.  ok.. that explanation sucks  //FIXME

Origin types:
 interp:  move in a straight line towards next camera point
 jump:  don't move at all towards next camera point, simply appear at the next one
 spline:  a 'best fitting' curve is created for all the camera points.  One of the disadvantages is that you are not likely to pass through the exact origin that you selected for a particular camera point.
 curve:  every three points of this type define a quadratic curve.  You can think of the first and last being the 'start' and 'end' points with the middle one controlling the steepness of the curve.  This has the advantage over the spline type of being able to pass through exactly through the chosen origins for camera points.
 splineBezier: (from q3mme, uses camera flags for masking)
 splineCatmullRom:  (from q3mme, uses camera flags for masking)

Angle types:
 interp:  linear angle interpolation
 spline:  (from q3mme)

Roll types:  There are options to treat roll as either part of the camera angles (ex:  airplane type movement) or as a camera tilting affect.

 interp:  roll is considered as separate from camera angles and changes linearly
 fixed:  roll is considered as separate from camera angles and stays at the current value
 pass:  roll is considered as separate from camera angles and skips this camera point when calculating interpolation
 angles:  roll is part of the camera angles and the setting is handled via the settings for camera angles

Velocity:
 //FIXME

 Note that spline fov uses the final fov values (ex: go from 60 -> 120 fov) instead of offset values which q3mme uses (ex:  10 -> 30  means go from currentFov + 10  to currentFov + 30).

  cvar cg_draw2d 2  for use with camera editing
    default  bind BACKSPACE "toggle cg_draw2d 0 1 2"

  cg_drawCameraPath
  cg_drawCameraPointInfo [0: disable camera point information, 1:  info for wolfcamql or q3mme camera points, 2:  info for wolfcamql camera points, 3:  info for q3mme camera points]
  cg_drawCameraPointInfo[X, Y, Align, Style, Font, PointSize, Scale, Color, SelectedColor, Alpha, BackgroundColor, BackgroundAlpha]

  cg_cameraRewindTime default 0  when playing a camera script will seek to this many seconds before the start of the camera path in order to allow animations and local entities (rail trails, wall marks, blood, smoke, etc...) to sync up and/or appear.

  cg_cameraQue to allow camera to play even without /playcamera command [1: don't play commands associated with camera points, 2: play camerapoint commands], can be in first person while playing camera and can hop in and out with /freecam
  cg_cameraAddUsePreviousValues  duplicates settings (camera type, etc.) of last added camera point when adding a new one

  cg_cameraDefaultOriginType  [ curve, spline, interp, jump ]

  cg_cameraUpdateFreeCam  transfers origin and angles to freecam state

  cg_cameraDebugPath  adds a model showing the camera path without updating freecam origin and angles.  Use r_drawworld 0  and r_fastsky 1  for a clearer view.  If both camera types are present (q3mme and wolfcamql) it will draw a model for each of them.

--------------------------------------------------------------------------

* camtrace 3d support:

  Add camera points as usual then /camtracesave [old]  will save cameras/ct3d/pos which can be imported into camtrace 3d.  They will be exported as et:qw compatible files unless the 'old' option is used.  'old' option writes etpro compatible files which can be imported into camtrace as obviously 'et' type and they can also be used in older versions of camtrace 3d.

  Commands added for compatibility are exec_at_time, g_fov, demo_scale, freecamlookatplayer [num] (FIXME: not implemented yet), freecamsetpos

  note: camtrace generated configs use the 'wait' command which is fps dependent and the camera can possibly speed up or slow down when recording.

  'wait [num]'  means wait for [num] screen draws, so:

  at 125 fps  'wait 1'  ==  wait 8ms
  at 30 fps  'wait 1' == 33.333_ ms

  So, if you have a camera that looks like you want it at 125fps and then you try to render a video at 30fps the camera will slow down.  If you try to render at 120fps with 20 blur frames the camera speeds up since effective fps is now (120 * 20 == 2400 fps)

  You can try playing the camera normally (without recording video) at whatever fps works well and use /recordpath command.  Then, when you record video, use /playpath command.

  You might need to copy wolfcamql.exe to et.exe and set camtrace's 'Enemy Territory' folder setting to point to wolfcamql's location.

---------------------------------------------------------------------------

* q3mme camera support:
    /q3mmecamera  (same as q3mme /camera command)
      added flags subcommand to print or set camera flag values:
      /q3mmecamera flags [origin | angles | fov | time]

      target sub command changed, you need to specify a target number (-1 means no target).  Ex:  /q3mmecamera target 9

    additional commands:

    /playq3mmecamera
    /stopq3mmecamera
    /saveq3mmecamera
    /loadq3mmecamera

    cg_q3mmeCameraSmoothPos is the same as mov_cameraSmoothPos in q3mme.

    FIXME target (Note:  target selection isn't available)

---------------------------------------------------------------------------

* cg_inheritPowerupShader  for multiple power-ups (ex: quad and medkit) do/don't apply the first power-up custom shader to the second

* +chat and cg_chatHistoryLength

* /addchatline

* rewind and fast forward using mouse:
    +mouseseek (bound by default to SHIFT)
    cg_mouseSeekScale  how quickly/slowly you change time
    cg_mouseSeekPollInterval  how often to check for mouse movement and then issue rewind/fast-forward call.
    cg_mouseSeekUseTimescale (1:  will scale demo seeking based on timescale value, 2: (default) take timescale value into account only when less than 1.0)

* framebuffer object support:  This lets you render into a separate framebuffer not associated with the application window.  This can prevent clipping if you are recording in a resolution greater than your screen size and allows you to use a small view window while recording in a higher resolution.

    r_useFbo
    r_fboStencil  (an extension to enable stencil shadowing with fbo)
    r_fboAntiAlias (number of anti-aliasing samples)
    r_visibleWindowWidth
    r_visibleWindowHeight

* You can record a demo while viewing one.
* voip data can be added to demo recorded from another demo (see voip-readme.txt)

  The client number for the new voip packets is 63 (MAX_CLIENTS - 1).
  If the original demo already contains voip data from the same client number,
  the playback will become corrupted.

* cl_voipOverallGain to control voip volume for all senders
* /voip gain also works with spatial clients

* r_portalBobbing

* r_teleporterFlash [0: draw black screen, 1: draw white screen, 2: wait to draw until in new view]

* cg_teamKillWarning  print "watch your fire!" for team kills

* cg_deathShowOwnCorpse  show your corpse falling to the ground when you die

* cg_deathStyle [0: don't alter view angles, 1: turn towards killer (like quake3 and quake live)(default), 2: keep tracking killer after you die, 3: set yaw to 0 (old behavior), 4: tilt camera like quake1]

* con_scale  increase/decrease size of console font

* con_lineWidth  Maximum number of characters in console line.  Default is "" which matches screen width.

* cg_obituaryFadeTime  milliseconds until obituary lines fade completely
* cg_obituaryStack  number of last obituaries to draw on screen

* cl_consoleAsChat 0  will let you issue console commands without having to add a '/' at the start

* cg_demoSmoothing

    1:  Look ahead in the demo to see if there are sequential snapshots where
        you have some type of velocity in both snapshots but your origin
        hasn't changed.  That means the server hasn't gotten around to playing
        the packets you sent, which will make demo playback jerky.  A cyan bar
        will be drawn in the lagometer whenever this happens.  Applies to demo
        pov as well as other players.
    2:  Smooth demo taker's pov using current and next snapshots (quake3
        default behavior is to just use the current snapshot origin and
        angles without smoothing).
    values higher than 2:  Skip this number of snapshots when smoothing.  The
        higher the value the more robotic and unnatural movement will become.
        Position will become more inaccurate and it's possible to clip through
        walls and trigger other similar bugs.

* cg_demoSmoothingAngles [0:  don't smooth view angles when using cg_demoSmoothing]
* cg_demoSmoothingTeleportCheck [0:  always transition player position when using cg_demoSmoothing]

--------------------------------------------------

* r_mapOverBrightBitsValue  r_overBrightBitsValue  as alternatives to r_mapOverBrightBits and r_overBrightBits

  I'll try to explain a bit what r_mapOverBrightBits and r_overBrightBits do:

  Both will multiply the colors of a texture by a power of 2, with r_overBrightBits using gamma ramp.  So r_mapOverBrightBits 0  will multiply the colors 2^0 == 1, doesn't change them at all.  r_mapOverBrightBits 1 will multiply by 2^1 == 2, r_mapOverBrightBits 2 will multiply by 2^2 == 4, r_mapOverBrightBits 3 will multiply by 2^3 == 8.

  Multiplying by powers of two doesn't offer very much control.  Personally I find r_mapOverBrightBits 2 (multiply by 4) to be a little dark, but with r_mapOverBrightBits 3 (multiply by 8) you basically obliterate all the light sources in the map since textures are as bright as they could possibly be.  Set r_mapOverBrightBits to 0 and then use r_mapOverBrightBitsValue to multiply it by whatever number you want.

  Incidentally r_mapOverBrightBits will preserve color.  It won't keep brightening until things will become white (unlike, i think, r_intensity), so if one of the color components (red green blue) hits the max value of 255 it doesn't brighten anymore.

  r_overBrightBits like I mentioned before applies gamma correction and it will distort colors. One of the limitations of quake3/ioquake3/etc is that gamma correction depends on r_ignorehwgamma being set to 0.

--------------------------------------------------

* F1  hard coded as an additional toggle for the console if you have problems using tilde ~

* binds attached to F1 -> F12 can be used even when the console is open

* cg_drawEntNumbers  1: draw the entity numbers above everything so you can easily id something like a rocket or grenade and then use /view <num> to lock the view onto it  2: with wall hack  3: with wall hack and also shows invisible entities

* cg_drawEventNumbers  draw floating id number of events
  note:  explosions are EV_MISSILE_MISS  (miss player), check console

* cg_drawPlayerNames  1: adds names above all players heads,  2: with wall hack

---------------------------------------------------------

* tokenized frag and obituary messages:
          cg_drawFragMessageTokens "You fragged %v"
          cg_obituaryTokens "%k %i %v"
          cg_drawFragMessageTeamTokens "You fragged %v, your teammate"
          cg_drawFragMessageThawTokens "You thawed %v"
          cg_drawFragMessageFreezeTokens "You froze %v"
          cg_drawFragMessageFreezeTeamTokens "You froze %v, your teammate"

Available tokens:
  %v  victim name
  %V  victim name with color escapes removed
  %k  killer name
  %K  killer name with color escapes removed
  %q  old q3 frag message ("player ate player2's rocket")
  %w  weapon name  ("Railgun, Rocket Launcher", etc..)
  %i  weapon icon
  %A  per kill total accuracy (all weapons)  (killer)
  %a  per kill accuracy of kill weapon (killer)
  %D  per kill total accuracy (all weapons) (victim)
  %d  per kill total accuracy of last held weapon (victim)
  %c  hex color code  ex:  %c0xff00ff  (note that the usual ^ color codes
          can still be used in the token string)
  %t  victim's team color
  %T  killer's team color
  %f  victim's flag in team games
  %F  killer's flag in team games
  %n  victim's country flag
  %N  killer's country flag
  %l  victim's clan tag
  %L  victim's clan tag with no color codes
  %x  killer's clan tag
  %X  killer's clan tag with no color codes
  %{newline <value>}  value is vertical distance
  %{x <value>}  horizontal offset value, note that text length for alignment
          is calculated without this offset
  %{y <value>}  vertical offset value
  %{font <point size> <font file/name>}  change fonts, this gets reset after
          newline
  %{scale <value>}  change font scale, this gets reset after newline
  %{iconscale <value>}  change icon scale, this gets reset after newline
  %{style <value>}  change text style, this gets reset after newline


  For lightning gun whores try:  cg_drawFragMessageSeparate 1, cg_drawFragMessageTokens "You fragged %v %a"

  For freezetag and thaw message use cg_drawFragMessageThawTokens

  cvar cg_fragTokenAccuracyStyle  (0: "35%", 1: "35", 2:  "0.35")

  Icon tokens are automatically scaled to the height of the surrounding text.  The type of scaling can be controled with cg_fragIconHeightFixed.

  cvar cg_fragIconHeightFixed  for frag and obiturary messages controls whether icon height is based on the height of the surrounding text (leads to variable icon sizes) or on a fixed height value per font (matches quake live)

  Scaling settings like %{iconscale ...} and cg_drawFragMessageIconScale are
  applied after automatic scaling based on text height.

---------------------------------------------------------

* preliminary support for combining multiple demos, still has a few bugs:
   /demo c1.dm_73 c2.dm_73 ...

   /set all_ents (1:  add all entities from all demos  2: extra debugging information)
     known bugs:  replaying event sounds for players


   temporary command '/pov next'   to cycle demos

----------------------------------------------------------

* cg_weather 0  to eliminate rain and snow

* cg_scoreBoardWhenDead  enable/disable automatically showing scoreboard when demo taker dies
* cg_scoreBoardAtIntermission  Draw scoreboard at the end of the game.
* cg_scoreBoardWarmup  draw scoreboard when you die in warmup
* cg_scoreBoardOld  use non premium scoreboards
* cg_scoreBoardForceLineHeight  override the menu defined line height for the player list in non-team modes.  (-1:  scales line height to fit all players, 0:  uses the menu defined value, > 0:  uses this value)
* cg_scoreBoardForceLineHeightDefault  this is the default number of players that fit in the non-team scoreboard, this needs to match menu value for line forcing to work
* cg_scoreBoardForceLineHeightTeam  override the menu defined line height for the player list in team modes.  (-1:  scales line height to fit all players, 0:  uses the menu defined value, > 0:  uses this value)
* cg_scoreBoardForceLineHeightDefault  this is the default number of players that fit in the team scoreboard, this needs to match menu value for line forcing to work
* cg_roundScoreBoard  in Attack and Defend game type
* cg_spectatorListSkillRating  show skill rating if known in the scrolling scoreboard spectator list
* cg_spectatorListScore
* cg_spectatorListQue


* demo looping  /setloopstart /setloopend /loop to start stop, or /loop <servertime start> <servertime stop>.  Check with /servertime command

* command /reseta cg_drawAttacker  to reset cg_drawAttacker* cvars to defaults

* command /fragforward [optional pre-kill time in seconds] [optional death hover time in seconds]

  Turn it off by simply issuing the /fragforward command again.
  '/fragforward stop' also explicitly stops it

  Autoexec files fragforwardnext.cfg and fragforwarddone.cfg are executed when
  seeking to next frag and when the last frag is played.

* cg_railItemColor for the rail guns that spawn or are dropped by players

* cg_railNudge 0  to let all rails pass through cross-hair

* cg_railRings  enable/disable spiral rails

* cg_railRadius cg_railRotation cg_railSpacing to control spiral rails

* cg_railQL to use cpma/quakelive rail core with rail rings interspersed, cg_railQLRailRingWhiteValue to control the brightness of the rings

* cg_enemyRailNudge and cg_teamRailNudge

* cg_railUseOwnColors

* cg_railFromMuzzle default 0  Will always make the rail origin the muzzle point.  It takes into account the different gun positions based on first and third person views and also takes into account any gun position changes made with the cg_gun* cvars.  It also passes the adjusted origin to the fx scripting system.

* cg_gunSize (first person) and cg_gunSizeThirdPerson

* cg_kickScale same as quakelive

* cg_crosshairColor "0xffffff"

* cg_drawCrosshairTeammateHealth*

* cg_drawCrosshairNames 2  to only show names for teammates

* cg_crosshairAlphaAdjust to adjust the transparent portions of cross-hairs.  cg_crosshairAlphaAdjust > 0 will make them more visible, less than 0 will make them even more transparent

* cg_crosshairBrightness values 0.0 -> 1.0 work the same as quake live (essentially making it more like cross-hair darkness).  Values greater than 1.0 will actually make it brighter.  The values above 1.0 will shift that integer amount of the pixels towards white.  cg_crosshairBrightness 255 will make all the original pixels white.

* cg_crosshairHitStyle 0-8 same options as quake live
* cg_crosshairHitTime, cg_crosshairHitColor

* command /cvarsearch <string> to find any cvar containing the string.  Ex:  /cvarsearch rail  will show the cg_rail* cvars as well as r_rail*

* command /listcvarchanges to show  what is different from default

* cg_drawRewardsMax (the number at which it only shows 1 icon with the count written below), cg_drawRewardsX, cg_drawRewardsY, cg_drawRewardsAlign, cg_drawRewardsStyle, cg_drawRewardsFont, cg_drawRewardsPointSize, cg_drawRewardsScale, cg_drawRewardsImageScale, cg_drawRewardsTime, cg_drawRewardsColor, cg_drawRewardsAlpha, cg_drawRewardsFade, cg_drawRewardsFadeTime

* cg_drawRewards: 1   (default) draw reward count to the right of icon, 2: draw reward count below icon like quake3, 3:  cpma style where multiple reward medals are shown

* cg_rewardsStack  (0:  don't play multiple reward announcements for the same reward if it happens within cg_drawRewardsTime)

* cg_fragMessageStyle controls placement string
      "You fragged SoAndSo"
       "1st place with 15"    <---

   (0:  never draw, 1: (default) only for the appropriate game types, 2:  always)

* option to separate frag messages from center print:  cg_drawFragMessageSeperate 1,  frag messages can then be controlled with cg_drawFragMessageX, cg_drawFragMessageY, cg_drawFragMessageAlign (0: align left, 1: align center, 2: align right), cg_drawFragMessageStyle (0: none, 3: shadow, 6: shadow even more), cg_drawFragMessageFont, cg_drawFragMessagePointSize, cg_drawFragMessageScale (this controls the size of the drawn text), cg_drawFragMessageTime (how long to stay on screen), cg_drawFragMessageColor (will not override white in player names), cg_drawFragMessageAlpha, cg_drawFragMessageFade (0: don't fade, 1: fade), cg_drawFragMessageFadeTime

* same options as above for cg_drawCenterPrint*

* cg_grenadeColor, cg_grenadeColorAlpha, cg_grenadeTeamColor, cg_grenadeTeamColorAlpha, cg_grenadeEnemyColor, cg_grenadeEnemyColorAlpha

* command /clearfragmessage


----------------------------------------------------


* cadd, csub, cmul, cdiv, ccopy   to modify cvars.  Ex:  /cadd timescale 0.5  to add 0.5 to current timescale value

* /cvarinterp /clearcvarinterp  to gradually change the value of a cvar
    ex:  /cvarinterp s_volume 0 0.7 6.0
         raises s_volume from 0 to 0.7 in 6 seconds

* branching options for configs:

  ifeq <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]

    ifeq (if equal)
    ifneq (if not equal)
    ifgt (if greater than)
    ifgte (if greater than or equal)
    iflt (if less than)
    iflte (if less than or equal)

    ex:
        /set cg_gibs 0
        /set vtrue "say gibs are on"
        /set vfalse "say gibs are off"
        /ifeq cg_gibs 1 vtrue vfalse

        player: gibs are off

    the above branching commands only work with integers, for decimal versions append 'f' to the branch name (ex:  ifeqf cvar 0.6 vtrue vfalse)

* cg_fadeColor, cg_fadeAlpha  for screen fading in/out
* cg_fadeStyle  (0:  fade before drawing hud, 1: fade after drawing hud)

* cg_drawFriend 1  has wall hack effect like in ql, cg_drawFriend 2 to disable wall hack, 3 (default) to enable wall hack effect in freezetag.  2010-12-17:  In quakelive it seems it's only in offline bot matches that wall hack effect is disabled.
* cg_drawFriendStyle  1 (default) friend icon color based on health, 2 yellow icon
* cg_drawFriendMinWidth  (like quake live)
* cg_drawFriendMaxWidth  (like quake live)
* cg_drawHitFriendTime   (like quake live)
* cg_drawDeadFriendTime  (like quake live)

* cg_drawFoe  draws icon over enemies heads.  2: with wall hack effect
* cg_drawFoeMinWidth
* cg_drawFoeMaxWidth

* cg_drawSelf similar to cg_drawFriend and cg_drawFoe.  It draws a white triangle over the demo pov and green triangle if demo pov is also the demo taker.  Useful if you use /follow
* cg_drawSelfMinWidth
* cg_drawSelfMaxWidth
* cg_drawSelfIconStyle  (0: use same icon regardless of team,  1: use diamond icon for enemy)


* cg_drawFlagCarrier  to control flag carrier icons (0: disabled, 1: (default) show only for team flag, 2:  show for both teams)
* cg_drawFlagCarrierSize  icon size
* cg_drawHitFlagCarrierTime  to control the time the hit icon is displayed

* cg_drawInfected  to draw 'bite' sprite over player's head in infected game type

* echopopup and echopopupcvar commands to print something on the screen.  /echopopupcvar <cvar> will only print the value.  Add 'name' if you want to print the cvar name as well.  Ex:  /echopopupcvar cg_teamModel name.  Position, time, and widescreen setting are controlled by cg_echoPopupX, cg_echoPopupY, cg_echoPopupTime, and cg_echoPopupWidescreen.

* +acc command to show both server side and client side weapon stats window.  Bound to CTRL by default.  Position controlled with cg_accX and cg_accY

* cg_screenDamageAlpha_Team, cg_screenDamage_Team, cg_screenDamage_Self, cg_screenDamageAlpha_Self, cg_screenDamageAlpha, cg_screenDamage

* demo rewind, fastforward, seek, seekend (from end of demo), seekclock (using the game clock)  Ex: /fastforward 60  to jump ahead 60 seconds, /seekclock\
 10:21 to go to 10:21
  /fastforward /rewind /seek /seekend can accept a cvar as arg and take the cvar's value
  /seekclock can seek within warmup.  ex:  /seekclock w5:12
  /seekclock can be passed in as a command line option (make sure +demo somedemo.dm_73 comes first)
  /seeknextround [millisecond offset]
  /seekprevround [millisecond offset]

cg_printTimeStamps  1: game clock time, 2: cgame time,  default is 0

* cl_freezeDemo [1/0]  to pause demo playback.  You can also use the pause command to toggle playback on and off.

* cl_freezeDemoPauseMusic to control whether music is stopped when demo is paused

* /wcstatsall  or /wcstats <player number> to show client side stats (k/d,
  accuracy, lag, etc.)

* /dumpstats  to print the stats data sent at the end of game in newer quake live demos

* scripted cameras  (/idcamera <cameraname>,  /stopidcamera)
     see http://rfactory.org/camerascript.html

* cg_drawClientItemTimer timer for major items (armors, mega health, quad, etc.)
     1 (the default)  appends an asterisk when the
     player is controlling the item, it also "fades" out after a few seconds
     and just shows "-".  Old behavior of always showing a time and staying
     stuck at zero can be enabled with cg_drawClientItemTimer 2

     customize with:  cg_drawClientItemTimerX, cg_drawClientItemTimerY, cg_drawClientItemTimerScale, cg_drawClientItemTimerTextColor, cg_drawClientItemTimerFont, cg_drawClientItemTimerPointSize, cg_drawClientItemTimerAlpha, cg_drawClientItemTimerStyle, cg_drawClientItemTimerAlign, cg_drawClientItemTimerSpacing, cg_drawClientItemTimerIcon, cg_drawClientItemTimerIconSize, cg_drawClientItemTimerIconXoffset, cg_drawClientItemTimerIconYoffset

* cg_drawClientItemTimerFilter to disable items.  Default is "rmygqb".  Available tokens are 'b' (battle suit), 'g' (green armor), 'm' (mega health), 'q' (quad damage), 'r' (red armor), and 'y' (yellow armor).  This also selects the order for drawing.

* cg_itemSpawnPrint  adds a chatline when items re-spawn and the demo was recorded with cg_enableRespawnTimer 1

* r_mapGreyScale, r_greyscaleValue   load the map in grey, darken to
  taste, and players + items will be colorized.  good value for
  r_greyscaleValue is 0.9

* r_picmipGreyScale "0"

Same as r_greyScale but only apply when picmip is allowed.


-----------------------------------------------------------

* libfreetype2 font support for cvars that display text in the hud
* /fontlist command
* utf8 support

  - r_debugFonts (0:  (default) no information, 1:  show missing glyphs,  2:  show font fallback checks, 3:  show all glyph loading)
  - font fallbacks:
      First the fonts included with quakelive are added to the font fallback list (notosans-regular.ttf and droidsansfallbackfull.ttf).  This can be enabled/disabled with r_defaultQlFontFallbacks.

      User set fonts are then added to the list.  These are specified as r_fallbackFont[number] (ex:  r_fallbackFont1, r_fallbackFont2, etc.).  If the cvar doesn't exist or is empty processing stops.

      Default Microsoft fonts (l_10646.ttf, segoeui.ttf, and arialuni.ttf)  are loaded from wolfcam's 'font' directory (either in base installation or application data directory).  These are the defaults used by quakelive and it lets non-Windows users have the same fallbacks by simply copying the fonts over.  This can be enabled/disabled with r_defaultMSFontFallbacks.

      System fonts are added for Windows users.  These are the same MS fonts listed above.  This can be enabled/disabled with r_defaultSystemFontFallbacks.

      GNU Unifonts are used as the final fallbacks.  This can be enabled/disabled with r_defaultUnifontFallbakcs.

  - to enter unicode characters into the console type 'x' plus the hex value and then press 'ctrl + tab' or 'ctrl + keypad_insert'.

* cg_qlFontScaling  [1: like ql, use alternate ql font when text becomes small, 2: always use 24 point ql font (old behavior)]

* cg_autoFontScaling and cg_autoFontScalingThreshold (minimum pointsize) to automatically increase and decrease font point sizes when scaling.  Doesn't apply to bitmap fonts.

* xy, font, align, etc.  cvars for all the stuff not covered by hud configs:  cg_clientItemTimerX,
  cg_clientItemTimerY, cg_drawFPSX, cg_drawFPSY, cg_drawSnapshotX,
  cg_drawSnapshotY, cg_drawAmmoWarningX, cg_drawAmmoWarningY,
  cg_drawTeamOverlayX, cg_drawTeamOverlayY, cg_lagometerX, cg_lagometerY,
  cg_drawAttackerX, cg_drawAttackerY, cg_drawSpeedX, cg_drawSpeedY,
  cg_drawOriginX, cg_drawOriginY, cg_drawItemPickupsX, cg_drawItemPickupsY,
  cg_drawFollowingX, cg_drawFollowingY, cg_weaponBarX, cg_weaponBarY,
  cg_drawCenterPrintY, cg_drawVoteX, cg_drawVoteY, cg_drawTeamVoteX,
  cg_drawTeamVoteY, cg_drawWaitingForPlayersX,
  cg_drawWaitingForPlayersY, cg_drawWarmupStringX, cg_drawWarmupStringY,
  wolfcam_drawFollowing

  All of the above can be enabled/disabled with the cvar without the appended
  x/y

  Example:  set cg_drawVote 0   // disable vote string
  Example: cg_drawFpsAlpha, cg_drawFpsColor, cg_drawFpsScale, cg_drawFpsPointSize, cg_drawFpsFont, cg_drawFpsStyle, cg_drawFpsAling, cg_drawFpsX, cg_drawFpsY, cg_drawFpsNoText (just shows the number)

  setting cg_*Font (ex: cg_drawFpsFont) to "" will use whatever the default font set in hud config
  quakelive font available as "fontimage"

  cg_*WideScreen cvars to control widescreen placement similar to quake live.  See the section for cg_wideScreen

-----------------------------------------------------------

* cg_drawTeamOverlayLineOffset  Specifies spacing between team overlay lines.  Value is percentage of line height.
* cg_drawTeamOverlayMaxPlayers  -1 (default) shows an unlimited number of players in team players.  A value greater than or equal to 0 sets the maximum number of players shown.  Note that the maximum number of players shown is also limited by the demo protocol.  Only quake live protocol 91 or greater can show more than 8.
* cg_selfOnTeamOverlay  Same as quakelive:  (0: (default) don't show yourself in team overvaly, 1: show yourself in team overlay)

* cg_scoreBoardStyle 0: like quakelive, 1 (default): switches the model icon for their best weapon icon, also prepends their accuracy before name,  2: switches model icon for country flags, also prepends their accuracy before name

* cg_shotgunImpactSparks

* cg_lagometerFlash  If your ping goes over cg_lagometerFlashValue and cg_lagometerFlash is set to 1, the background color of the lagometer changes to orange.  The default value for cg_lagometerFlashValue is 80 since that's the max ping that quake live servers will anti-lag.

* cg_drawItemPickups same options as quakelive

* cg_weaponBar quakelive/cpma style weapon bar, same values as quakelive,
    except for one change cg_weaponBar 5 is like cg_weaponBar 4 (weaponSelect)
    which doesn't fade off the screen.

* cg_drawAmmoWarning (0: no message,  1: message for low and out of ammo,  2: message for no ammo)

* cg_lowAmmoWarningStyle (0:  broken q3 style based on total ammo of all weapons and estimate of seconds left, 1:  like quake live based on percentile, 2:  based on per weapon cvars)

* cg_lowAmmoWarningPercentile  like quakelive
* cg_lowAmmoWarningSound  like quakelive
* cg_lowAmmoWeaponBarWarning  like quakelive

* the following are used with 'cg_lowAmmoWarningStyle 2':
  cg_lowAmmoWarningGauntlet, cg_lowAmmoWarningMachineGun,
  cg_lowAmmoWarningShotgun, cg_lowAmmoWarningGrenadeLauncher,
  cg_lowAmmoWarningRocketLauncher, cg_lowAmmoWarningLightningGun,
  cg_lowAmmoWarningRailGun, cg_lowAmmoWarningPlasmaGun, cg_lowAmmoWarningBFG,
  cg_lowAmmoWarningGrapplingHook, cg_lowAmmoWarningNailGun,
  cg_lowAmmoWarningProximityLauncher, cg_lowAmmoWarningChainGun

* cg_ambientSounds
    0:  disables all map sounds (not recommended)
    1:  play all map sounds
    2:  (default)  disables ambient sounds except for various important sounds like powerup respawn, doors opening and closing, special item respawns, etc..
    3:  (like quakelive's 's_ambient 0') disables map looping sounds like fire, wind, etc. and allows map triggered sounds

* cg_hitBeep  multi-tone hit-sounds, same values as quakelive

* exec files: automatically execs the following

  - follow.cfg when following player (to allow different huds)
  - freecam.cfg when switching to freecam
  - spectator.cfg when you switch to spectator
  - ingame.cfg when you switch back to your own view
  - gameend.cfg
  - gamestart.cfg
  - roundend.cfg
  - roundstart.cfg
  - firstpersonswitch.cfg  when switching first person view to another player (ex:  spectator demo switches person being viewed)
  - wolfcamfirstpersonswitch.cfg  same as firstpersonswitch.cfg but using /follow
  - wolfcamfirstpersonviewdemotaker.cfg  when /follow'ed player is same as demo taker view
  - wolfcamfirstpersonviewother.cfg  when /follow'ed player is not the same as demo taker view
  - scoreboardon.cfg
  - scoreboardoff.cfg
  - cgameinit.cfg  just before map load, most game info and commands not available
  - cgamepostinit.cfg  all set up has been completed and all the game info and commands are available
  - demoTakerDieRound.cfg  if demo taker is also a player and dies during a Clan arena or CTFS round

* exec files: will exec a per gametype config (duel.cfg, ffa.cfg, tdm.cfg, ca.cfg, ctf.cfg, 1fctf.cfg, obelisk.cfg, harvester.cfg, freezetag.cfg, domination.cfg, ad.cfg, rr.cfg, (cpma) ntf.cfg, (cpma) 2v2.cfg, (cpma) hm.cfg, iffa.cfg, ictf.cfg, i* etc..)

* exec files: map autoexec:  (maps/campgrounds.cfg, maps/lostworld.cfg, etc...)

  note:  maps/campgrounds.cfg and maps/qzdm6.cfg  contain a fix for missing pillar models/textures when using freecam

* ql hud area chat:  cg_chatTime, cg_chatLines

* cg_drawSpawns,  2 enables wall hack effect.  Drawn in red, with the
    preferred initial spawns in yellow/gold

* cg_drawSpawnsInitial  Used with cg_drawSpawns 1 to show which spawns are marked as possible starting points in a duel game.

* cg_drawSpawnsInitialZ

* cg_drawSpawnsRespawns  Used with cg_drawSpawns 1 to show which spawns as possible re-spawn points after the game has started.

* cg_drawSpawnsRespawnsZ

* cg_drawSpawnsShared  Used with cg_drawSpawns 1 to show which spawns points can be used by both the red and blue teams.

* cg_drawSpawnsSharedZ

   ex: "cg_drawSpawns 1;  cg_drawSpawnsInitial 1;  cg_drawSpawnsRespawns 0;  cg_drawSpawnsShared 0"     to only show initial spawns

   ex: "cg_drawSpawns 1;  cg_drawSpawnsInitialZ 20;  cg_drawSpawnsRespawnsZ 40"   to draw initial spawns 20 units higher than normal and re-spawns 40 units higher than normal so that a possible overlap can be seen

--------------------------------------------------------------------------
freecam   type /freecam in console
  use your bound movement keys

  cvars:
    cg_freecam_crosshair    enable/disable a crosshair
    cg_freecam_noclip    pass through walls
    cg_freecam_sensitivity    mouse sensitivity
    cg_freecam_yaw    additional right/left mouse sensitivity
    cg_freecam_pitch    additional up/down mouse sensitivity
    cg_freecam_speed    movement speed

    cg_freecam_unlockPitch    quake3 normally locks your view upwards.  If you
        move your mouse so that you look upwards the view will stop changing
        when you are looking directly upwards even if you keep moving the mouse
        in the upward direction.  Unlocking pitch will let you keep changing
        upward angles until you eventually loop around.

    cg_freecam_useServerView    automatically turn off matching demo taker's
        screen (cg_useOriginalInterpolation 1) since it's pointless in third
        person.  Default is 1.  This also applies when using cg_thirdPerson.

    cg_freecam_useTeamSettings  1: set demo takers skin/model to match teammates  2:  same as 1, but also use pov settings (ex:  showing flag taker capture point)

  autoexecs freecamon.cfg and freecamoff.cfg

  locking view to an entity or position:

      /view <entity num> [x offset] [y offset] [z offset]    // [] are optional
          or
      /view here    // lock view to current freecam position


      Use '/view' by itself to unlock.

      Use /listentities or 'cg_drawEntNumbers 1' to get entity numbers.

      /viewunlockyaw   // allow left/right view changes but keep pitch locked
      /viewunlockpitch    // allow up/down view changes but keep yaw locked

      Ex:  use /viewunlockpitch if you want to view someone strafe jumping and not have the view bounce up and down.

  /gotoview [forward] [right] [up] [force] relative to the viewed entity (or demo taker).  The force (integer) option means to jump to the view even if you might get stuck inside a wall.  []  are optional

  /stopmovement for freecam since there's no gravity friction
  command /recordpath [filename] to save your position and viewangles to a file.  /recordpath will also stop a current recording.  To play back use /playpath
  /stopplaypath to stop playback
  cg_pathRewindTime  (milliseconds: extra time to rewind to allow things like explosions, wall marks, rail trails, etc..  to be drawn)
  cg_pathSkipNum (smoothing value -- how many points to skip and smooth between)

freecam +rollright +rollleft /centerroll (like /centerview) +rollstopzero
  These change screen tilt.  You can use +rollstopzero in order to recenter it smoothly while using +rollright or +rollleft.  defaults:
      bind e "+rollright"
      bind q "+rollleft"
      bind f "+rollstopzero"
      bind 3 "centerroll"


/chase <entNum> [x offset] [y offset] [z offset]     [] means optional

/move command changes positions/angles relative to where you already are:  /m\
ove [forward amount] [right] [up] [pitch] [yaw] [roll]

/moveoffset same as move but using xyz coordinates

/freecam accepts [offset | set | move | last] as a first argument (with offset being the default)
  ex:  /freecam move 100   switch to third person 100 game units ahead of player
       /freecam offset 0 0 26  switch to third person 26 game units above player
       /freecam last   switch to third person and use last freecam position
---------------------------------------------------------------------------

Player Models:

* color skins for all models, they are generated automatically from the already available red and blue skins you can use r_colorSkinsFuzz (the amount that the difference between red and blue skins will signal a match and not replace with white: default 20)  and r_colorSkinsIntensity (default 1.0) and the /createcolorskins  vid_restart to tweak them.  You can then use skin overrides to allow either team or enemies to keep their models but use colorized skins:    ex:  set cg_enemyModel "", then cg_enemyHeadSkin "color", cg_enemyTorsoSkin "color", cg_enemyLegsSkin "color", same for team

* cg_ignoreClientHeadModel [0: use head model set by player,  1: ignore and use 'model' setting,  2: (default) ignore for quake live demos and use 'model' setting]

* cg_disallowEnemyModelForTeammates [0, 1, 2]   If cg_teamModel isn't set and teammates use their own models this will prevent them from using the model set with cg_enemyModel.  The replacement model is set with cg_fallbackModel and cg_fallbackHeadModel.  With a setting of 1 it will also apply to first person view.  With a setting of 2 it only applies to teammates and not the person being viewed.

* cg_fallbackModel "crash"
* cg_fallbackHeadModel "crash"

* cg_useDefaultTeamSkins  If you don't have forced team models you can set to 0 and see your teammates chosen skin instead of blue or red.

* "-teamColor" skin to pick appropriate blue, red, or default skin.

  Example:  This can be used with cg_useDefaultTeamSkins 0 to then select red/blue for either/or enemies or teammates.

* cg_playerLeanScale  similar to quakelive's cg_playerLean and allows you to select how much leaning the player models do (can be higher than 1.0).

* cg_deadBodyColor "0x101010"

* player model scaling and server set models:
    cg_playerModelAutoScaleHeight  scale player models to match this height (default: 57, "" to disable)

    these are used after auto scaling pass:
      cg_playerModelAllowServerScale  use server info to apply additional scaling
      cg_playerModelAllowServerScaleDefault  for older demos without server info use this as default  (default is 1.1)


    server set player models:
      cg_playerModelAllowServerOverride  (default 1)

    options to force settings:
      cg_playerModelForceScale, cg_playerModelForceLegsScale, cg_playerModelForceTorsoScale, cg_playerModelForceHeadScale, cg_playerModelForceHeadOffset

      client override options:  "modelscale", "legsmodelscale", "torsomodelscale", "headmodelscale", "headoffset", "modelautoscale" (match this height)

* cg_forcemodel has a new option 2:  ignore team skins
* cg_forcePovModel  will use 'model' and 'headmodel' settings for 1st person pov

  1:  also sets team skins and colors
  2:  sets model and ignores team settings

  If model or headmodel are set to "" it will use the original player values.
  Setting to "" can be used with cg_forcePovModel 1 to use team settings (team colors, skins, fallback if enemy model) for demo taker's model.

* cg_forcePovModelIgnoreFreecamTeamSettings  can be used to override the value of cg_freecam_useTeamSettings
* cg_ourHeadSkin to set alternate skin with cg_forcePovModel 2
* cg_ourTorsoSkin to set alternate skin with cg_forcePovModel 2
* cg_ourLegsSkin to set alternate skin with cg_forcePovModel 2
* cg_ourHeadColor set color with cg_forcePovModel 2
* cg_ourTorsoColor set color with cg_forcePovModel 2
* cg_ourLegsColor set color with cg_forcePovModel 2

* cg_enemyModel "keel/bright"
* cg_enemyHeadModel "keel/bright"

  For both cg_enemyModel and cg_enemyHeadModel setting to "" will use original player values.  Setting to just a model without skin (ex: "doom") will pick the appropriate red or blue skin in team games.

* cg_enemyHeadSkin ""  can be used to override what is set with cg_enemyHeadModel.
    Ex:  cg_enemyHeadSkin "xaero/color" and cg_enemyHeadModel "sarge/default" to use xaero's color head skin on sarge's head model

* cg_enemyTorsoSkin
* cg_enemyLegsSkin

* cg_enemyHeadColor   hex value  ex:  "0x00ff00"
* cg_enemyTorsoColor
* cg_enemyLegsColor


  To change enemy or team colors without changing models you can use something like:
      cg_enemyHeadSkin "bright"   // without specifying a model
        or
      cg_enemyHeadSkin "color"    // without specifying a model
      ...


* cg_enemyRailColor1
* cg_enemyRailColor2

* cg_enemyRailColor1Team  (use team color)
* cg_enemyRailColor2Team  (use team color)
    For both cg_enemyRailColor1Team and cg_enemyRailColor2Team the colors are set using cg_weapon[Red|Blue|No]TeamColor  which is a hex value

* cg_enemyRailItemColor  the color of the actual gun they hold, default is "" which uses whatever the player's color1 is.
* cg_enemyRailRings

* for cg_team* same options as cg_enemy*

* cg_wh [0:  no wall hack, 1:  wall hack shader applied to all players, 2:  wall hack shader applied only to enemies, 3:  wall hack shader applied only to teammates]

* cg_whIncludeDeadBody  // 1 or 0, includes dead bodies
* cg_whIncludeProjectile  // 1 or 0, includes projectiles
* cg_whShader if you want to use something other than 'wc/wallhack'
* cg_whColor
* cg_whAlpha
* cg_whEnemyShader
* cg_whEnemyColor
* cg_whEnemyAlpha

* cg_playerShader to add a non wall hacked shader (can be used in combination with cg_whShader to make it easier to see when someone becomes visible)

* cg_itemsWh  wall hack for items
* cg_itemSize
* cg_itemFx same as quake live

* quake3 demo playback support
   ex:  copy map_cpm3a.pk3 into wolfcamql/wolfcam-ql/
        wolfcamql.exe +demo ratelpajuo

* cg_useOriginalInterpolation "0"  // 0: try to match demo taker's screen (including ping correction), 1: same as q3/ql

Setting to 0 will try to match the demo taker's screen with respect to player and projectile positions.  It compensates for both ping and the fact the actual game-play involved predicting your own movement (see cg_nopredict).  Setting it to 1 will play back demos in the same way that other quake3 based games play them, showing what the server saw and not the client, and will always show the player's aim behind were it actually was.

* wolfcam  cg_interpolateMissiles "1"

One of the side effects of using cg_useOriginalInterpolation 0, is that it will introduce lag with higher pings (ex: delay between rocket fire sound and when the projectile appears).  If you want to only match player positions and take out that delay you can use cg_interpolateMissiles 0.  The side effect is that projectile positions will be less accurate.


* cg_chatBeep "1"          // 1:enable, 0:disable
* cg_chatBeepMaxTime "4.5" // don't play beeps quicker than this many secs.  It let's you have chat beeps without being annoyed by someone with a spam bind.

* cg_teamChatBeep
* cg_teamChatBeepMaxTime  don't play team chat beeps faster than this (in seconds)".  It let's you have team chat beeps without being annoyed by someone with a spam bind.

* cg_serverPrint  to enable/disable printing server messages in the center of the screen
  cg_serverPrintToChat  to enable/disable printing server messages in the chat area
  cg_serverPrintToConsole  to enable/disable printing server messages in the console

  in quakelive 'server print' messages include things like 'player disconnected', 'vote failed', and ctf flag status messages

* cg_serverCenterPrint*  (see cg_serverPrint*)

  in quakelive 'server center print' messages include things like 'player switched teams'

* cg_drawCpmaMvdIndicator  show on screeen message if demo taker view is using cpma mvd mode

* cg_drawFollowing:  2 will always draw the following text even if it's the demo taker

* cg_printSkillRating  Add a message to the chat area indicating a player's skill rating when they connect or disconnect.

* /clearscene to remove marks and local entities (rail trails, wall marks, blood, smoke, etc...)

* cg_allowSpritePassThrough default 1   wont delete sprites if you move through them
* cg_allowLargeSprites default 0,  if a sprite is big enough and you are close to it the quake3 engine won't draw it.
* cg_drawSprites  (like quake live)
* cg_drawSpriteSelf  (like quake live:  controls drawing own 'frozen' sprite in freezetag)
* cg_drawSpritesDeadPlayers controls drawing sprites for dead players

* cg_hasteTrail  to allow or disable smoke trails
* cg_flightTrail

-----------------------------------------------
+vstr
     /set rDown echopopup r key down
     /set rUp echopopup r key up
     /bind r +vstr rDown rUp

++vstr   which takes a repeat value in milliseconds
     /set rDown play sound/weapons/lowammo
     /set rUp echopopup r key up
     /bind r ++vstr 200 rDown rUp

-----------------------------------------------

* alias, unalias, and unaliasall  same as quake live

* cg_thawGibs  number of gib particles for freezetag thaw effect
* cg_gibColor  for the color of the gib particle emitted
* cg_gibSparksColor  for the color of the spark trail
* cg_gibSparksSize
* cg_gibStepTime  How often, in milliseconds, to leave a glowing spark trail.
* cg_gibSparksHighlight  Draws a single white pixel at the center of the spark to make things a little more visible.

* cg_impactSparksColor
* cg_impactSparksHighlight  Draws a single white pixel at the center of the spark to make things a little more visible.

* cg_simpleItemsScale
* cg_simpleItemsBob and cg_simpleItemsHeightOffset same as quakelive

* cg_smokeRadius_PL, cg_smokeRadius_RL, cg_smokeRadius_NG, cg_smokeRadius_GL, cg_smokeRadius_SG

* cg_waterWarp

-------------------------------------
r_singleShader options and r_singleShaderName  to replace all the map textures

  ex:  +set r_singleShaderName textures/crn_master/smooth_concrete +set r_singleShader 2

  r_singleShader options:  1 replace all textures include advertisements and don't apply lightmap, 2 same as 1 but apply lightmap, 3 replace all map textures except advertisements and don't apply lightmap, 4 same as 3 but apply lightmap

  Along with r_fastSkyColor can be used for chroma keying:

  +set r_singleShaderName wc/map/custom +set r_singleShader 1 +set r_fastSky 1 +set r_fastSkyColor 0x010001

  You can edit or replace wolfcam-ql/gfx/wc/custom.tga to choose the color.
--------------------------------------

* 'screenMap' shader keyword from Quake3e.  This uses a screenshot of the previous frame as a texture.  Can be used for mirror like surfaces.  The size of the texture used can be controlled with r_screenMapTextureSize.  Example

  models/weapons2/rocketl/rocketl
  {
          cull none
          {
                  map models/weapons2/rocketl/rocketl.tga
                  rgbGen lightingDiffuse
          }
          {
                  screenMap
                  rgbGen lightingDiffuse
                  tcGen environment
                  blendfunc add
          }
  }

  Note:  Unlike Quake3e, 'screenMap' doesn't accept an image fallback option and 'tcGen environment' doesn't have a 'firstPerson' option.

* r_fastSkyColor  hex:  0xff0000  if using r_fastSky 1
* r_forceSky

  ex:  +set r_forceSky textures/skies/hellsky

* r_fastsky 2  same as 1 but wont disable portal views
* r_drawSkyFloor same as quakelive
* r_cloudHeight with the original value stored in r_cloudHeightOrig.  To disable:  set r_cloudHeight ""

* r_mapOverBrightBitsCap same as quakelive
* r_darknessThreshold  control the brightness of map shadows and dark areas without changing too much the color and brightness levels of the lit portions

  With opengl2 this doesn't work with hdr lightmaps.

* r_BloomTextureScale similar to r_BloomBlurScale.

When you implement bloom you take a screen-shot, apply blur, filter by an intensity threshold, and then re-blend with original image.  r_BloomTextureScale controls how small the screen-shot that you are going to process will be.  By using a smaller image you improve performance and it also acts as a type of blurring.

r_BloomBlurFalloff and r_BloomBlurRadius still not implemented, but the following work: r_enableBloom r_BloomPasses r_BloomSceneIntensity r_BloomSceneSaturation r_BloomIntensity r_BloomSaturation r_BloomBrightThreshold


* r_enablePostProcess, r_enableColorCorrect, r_contrast same as quakelive

* con_transparency, con_fracSize, con_conspeed
* con_rgb  ex:  'con_rgb 0x200000' to replace console shader with just a dark red color.  Default is con_rgb ""

* cg_shotgunStyle [0: quake3  1: quake live cg_trueShotgun  2: ql shotgun with randomness]
* cg_shotgunRandomness to control how much 'cg_shotgunStyle 2' diverges from true shotgun pattern

--------------------------------------------
/at  command

  usage:  at <'now' |  server time  |  clock time> <command>
    ex:  at now timescale 0.5    // current server time
         at 4546629.50 stopvideo    // server time
         at 8:52.33 cg_fov 90       // clock time
         at w2:05 r_gamma 1.4    // warmup 2:05

  cvar cg_enableAtCommands
  /listat
  /clearat
  /removeat (number)
  /saveat <file.cfg>  // can then be execed

  note:  for clock values currently only supports rising level timer direction

------------------------------------------------

* +info command to show server info.  Bound by default to ALT

* cg_lightningRenderStyle [0: like q3/ql lg beam embeds into objects,  1: lg beam with depth hack drawn over objects], cg_lightningAngleOriginStyle [0: like q3 ignoring step adjustments,  1:  like ql/cpma beam takes out client side screen effects like step adjustment and damage kick,  2: use player angles and origin without client side effects]

* cg_lightningSize  lg beam width
* cg_lightingShaft same as quake live
* cg_lightingImpact

* cg_lightningImpactSize

* cg_lightningImpactCap (same as quakelive - don't draw the impact point any bigger than it is at this distance with respect to screen size)

This was presumably added so that the impact point doesn't cover most of your screen when you are fighting up close or clip a wall.  The default for quake live is 192.  Use cg_debugLightningImpactDistance 1 and then /devmap <somemap> and find a spot where the distance is 192.  Use a ruler and measure on your monitor how big the impact image is.  If you move closer to the impact point while still firing lg you'll notice that the impact point still only takes up the same amount of screen space as when it was at a distance of 192.


* cg_lightningImpactCapMin (to make sure it doesn't draw smaller than this)

Same logic as cg_lightningImpactCap but makes sure than the impact point doesn't become too small.

* cg_lightningImpactProject (push the impact point outwards by this amount)

It pushes the impact location this amount towards you in order to increase visibility.  Quake3 pushes it out by 16 units, quake live seems to be about 1.

* cg_debugLightningImpactDistance (prints distance from you to impact point in console)

* cg_lightningImpactOthersSize  size of lg impact when demo taker or pov get's hit

* cg_warmupTime [0: draw 0 in clock, 1: draw time in warmup and '(warmup)' string, 2: draw 0 and '(warmup)' string, 3: draw only the time]

* cg_drawItemPickupsCount  to enable/disable new quake live behavior

* cg_fovStyle

	0:  Same as quake3.  cg_fov sets the actual horizontal fov and the vertical fov is adjusted according to this.  This avoids stretching the ingame world view but has the effect of zooming in as the screen width increases.

	1:  (default) This preserves vertical fov as if the screen dimensions were 4:3.  Horizontal fov is adjusted according to this.  This avoids stretching the ingame world view and has the effect of showing more of the horizontal view when the screen width increases.

	2:  Same as quakelive.  This is similar to 1 but uses fixed aspect ratios (5:4, 4:3, 16:10, and 16:9).  Vertical fov is preserved and more of the horizontal view is shown as the screen width increases and passes through each aspect threshold.  This stretches the ingame world view to match the highest apsect ratio matched and has an upper limit of 16:9.


* cg_fovy   to force a y fov

* cg_fovForceAspectWidth and cg_fovForceAspectHeight  with cg_fovStyle 1 or 2 this forces fov calculations to this apsect ratio and then stretches the ingame world view to match the screen width.

* cg_fovIntermission  force a default fov during intermission, default is 90

* cg_enableBreath [0: never, 1: if server/demo has enabled, 2: if map has it enabled even if server/demo don't, 3: always]

* cg_enableDust [0: never, 1: if server/demo has, 2: if map has it enabled even if server/demo don't, 3: always if surface is dust, 4: always even if surface isn't dust]

* cg_smokeRadius_dust, cg_smokeRadius_breath, cg_smokeRadius_flight, cg_smokeRadius_haste

* cg_drawgun  2:  eliminates bobbing like quake live, 3:  transparent weapon, 4:  transparent weapon with bobbing

* cg_localTime (0: use demo time, 1: real time) default 0, cg_localTimeStyle (0: 24-hour clock, 1: am, pm) default 1

* cg_itemUseSound, cg_itemUseMessage, cg_noItemUseMessage, cg_noItemUseSound

* cg_qlhud  0: use quake3 hud, 1: use quake live scripted hud

* cg_drawTeamBackground  Enable/disable drawing the team colored bar at the bottom of the hud in the original quake3 hud (cg_qlhud 0).

* cg_drawScores cg_drawPlayersLeft cg_drawPowerups to enable/disable in the original quake3 hud (cg_qlhud 0).

* cg_testQlFont  Replaces all uses of quake3 mono-space font with quakelive's font.  Used for testing.

* wolfcam  cg_perKillStatsExcludePostKillSpam "1"

With frag message tokens you can display your weapon accuracy for that kill:
set cg_drawFragMessageTokens "You fragged %v with %a accuracy"

cg_perKillStatsExcludePostKillSpam 1  will reset per kill accuracy when you stop firing not when you kill someone.  That way those extra shots don't carry into the stats for the next kill.

* wolfcam  cg_perKillStatsClearNotFiringTime "3000"

For per kill accuracy stats they will be reset if this amount of time (in milliseconds) passes and you haven't fired your weapon.

* wolfcam  cg_perKillStatsClearNotFiringExcludeSingleClickWeapons "1"

If this is set to 1 only weapons that are usually used by holding down +attack get a stat reset if the time amount in cg_perKillStatsClearNotFiringTime passes.  (ex:  rail, rocket launcher, etc.)

* in_nograb

You can use it in order to un-grab the mouse pointer without having to bring down the console.  Toggling it is bound by default to F2.  So, for example, you could start rendering a demo, hit F2, minimize the window, and then go off and do whatever until the rendering is done.

* cg_drawJumpSpeeds*  [1: clear when velocity close to zero (like q3 defrag), 2
: don't automatically clear]

  /printjumps
  /clearjumps

* sv_broadcastAll  (test cvar)

* /entityfilter <'clear', 'all', TYPE, entity number>

  Removes entities.  TYPE can be: general, player, item, missile, mover, beam, portal, speaker, pushtrigger, teleporttrigger, invisible, grapple, team, events.

  /listentityfilter

* /entityfreeze <'clear', entity number>

  /listentityfreeze

* /eventfilter <'clear', event id>
   see cg_drawEventNumber

   /listeventfilter

* cg_killBeep   same as quake live

* fs_quakelivedir  Default quake live install paths are checked in order to list demos (steam and stand alone versions).  You can override with this cvar.  It should point to the directory that contains the 'baseq3' directory where demos are saved.  Ex:

   # steam version
   wolfcamql +set fs_quakelivedir "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Quake Live\\12345678901234567"

   # stand alone version
   wolfcamql +set fs_quakelivedir "C:\\Users\user1\\Application Data\\LocalLow\\id Software\\quakelive\\home"

* r_dynamicLight  1 fixes square lights, 2 original q3 code (sometimes textures have light added to the wrong side), 3 testing only determine if lighting applies in one place in the source code

  Not needed with cl_renderer opengl2.

* shader override  In <app data>/wolfcam-ql place them in shaderoverride.

  ex:  shaderoverride/blah.shader

* 'wcrealtime' general shader keyword added.  This allows shader time updated using real time instead of time adjusted for timescale, pause, and video frames.

  This was added mostly to allow the console shader to run at the same rate regardless of the settings listed above.

* quake live 'novlcollapse' shader option to prevent ignoring of lightmaps with r_vertexLight option

* cg_quadKillCounter (same as quakelive)
* cg_battleSuitKillCounter (same as quakelive)

* cg_wideScreen

  Note:  Quake Live's August 27, 2014 update added support for widescreen
         huds.  That is enabled with value 5 (default).  Quake Live's
	 widescreen settings pretend the hud has a 4:3 (width / height) ratio
	 simply based on the screen height and that elements can be drawn in
	 one of three locations:

               1                     2                     3

         ______________        ______________        ______________
	 |****        |        |    ****    |        |        ****|
	 |****        |        |    ****    |        |        ****|
	 |****        |        |    ****    |        |        ****|
	 --------------        --------------        --------------

         Imagine that the extra widescreen space were treated as black
	 bars and that the hud was drawn three times.

         The default widescreen value in menus appears to be '2' if it's never defined in a menuDef.

  Values:

        0: original code q3 (and older quake live versions), with images stretched or shrunk horizontally

        1: don't adjust values to 4/3 aspect ratio, it will use the exact values specified

           Note:  Hud and cvar position cvars are normally based on a 640x480
           virtual screen and will stretch images and text if the screen width
           isn't 1.333_ larger than the screen width.  '1' for this cvar
           disables that behavior.

        2: adjust both y and x values based on only the x ratio (a bit of a hack to allow already created configs)

        3: only adjust the crosshair, similar to quake live

	4: test setting using aspect ratio

	5: Quake Live style widescreen.  See description above. (default)


  This isn't perfect as there are still some hard coded values assuming 4/3 aspect within the source code, but it should work well for movie configs to prevent stretching of the crosshair and text.


* cg_wideScreenScoreBoardHack  (1: don't stretch but center on screen, 2:  ignore cg_wideScreen and stretch to fit)
  This doesn't apply if 'cg_wideScreen 5' is used.

* r_anaglyph2d to allow or prevent splitting colors for the hud portion
* r_anaglyphMode 19 uses full color for both left and right channels

* cg_damagePlum  same as quakelive (string listing weapons that will show damage plums)  Also allows 'none' as token for kills without weapon numbers.  Ex:  "none g mg sg gl rl lg rg pg bfg gh cg ng pl hmg"
* cg_damagePlumColorStyle  same as quake live (1: colored from white to red based on damage, 2:  crosshair hit colors for different damage amounts, 3:  colored based on weapon)
* cg_damagePlumTarget (0: none, 1: (default) only for the viewed player, 2: all players)
* cg_damagePlumTime  how long the damage plums last before they fade away
* cg_damagePlumBounce  initial upward velocity
* cg_damagePlumGravity  downward pull velocity
* cg_damagePlumRandomVelocity  random range for extra left/right + forward/back velocity
* cg_damagePlumFont
* cg_damagePlumFontStyle
* cg_damagePlumPointSize
* cg_damagePlumScale
* cg_damagePlumColor  if this is set it will override colors from cg_damagePlumColorStyle
* cg_damagePlumAlpha

* cg_drawFPS (2: higher precision and use given time in cgame not real time -- for debugging,  3:  use current frame value not average of last four)
* cg_autoWriteConfig (0: don't automatically write q3config.cfg when a cvar changes (can be useful for testing configs), 1:  always write q3config.cfg when a cvar changes, 2:  (default) don't write q3config.cfg if a cvar is changed from fx scripting code or with /cvarinterp to prevent hard disk thrashing)

* cg_adShaderOverride  when set to 1 you can use cg_adShader[num] to replace the ad.  It needs to be the name of a shader:
  ex:  create a file name scripts/mycustomads.shader  with the following:

  myad1 {
    nopicmip
    {
      map somePic.jpg

      // or play a video (if directory isn't specified looks in 'video/') :
      //videoMap video/intro.roq
    }
  }

  Then 'set cg_adShader3 myad1' will replace the 3rd map ad with your custom pic.
  You can use /devamp somemap  and then /gotoad [ad number]  to find how they are numbered.  cg_debugAds can also be used to locate advertisements.

* cg_debugAds  adds depth hacked in game numbers to locate specific ads
* cg_decal[num] to add custom decals/posters
  format is:  place to add decal, some point in front of the decal, orientation/angle, red, green, blue, alpha, size, shader name

  example on campgrounds map:
      /set cg_decal1 "-392 -262 1  -284 -427 282  0 1 1 1 1 90 wc/poster"
      draws a pentagram next to quake live logo in quad/mega room
      (-392, -262, 1)  decal origin
      (-284, -427, 282)  decal faces this point

      you can use cg_debugImpactOrigin to test origin placements

      also for testing you can use /adddecal <shader>
      ex:  bind n "adddecal wc/poster"
           shoot at a wall, press 'n', and check cg_decal* values

* cg_forceBModel[number]
  cg_forceBModelPosition[number]
  cg_forceBModelTrajectory[number]

  force the display of server set models in maps

  position format is "(float)origin[0] (float)origin[1] (float)origin[2]
    (float)angles[0] (float)angles[1] (float)angles[2]"
    ex: "100.0 -20.0 300.0 0.0 0.0 0.0"

  trajectory format is "(int)type (int)time (int)duration (float)velocity[0]
    (float)velocity[1] (float)velocity[2]"  ex:  "4 0 5000 0.0 0.0 4.0"
    times are in milliseconds

  example to fix missing pillar texture/models in campgrounds pillars when freecaming and demo taker is out of range:

  ffa and duel:
      set cg_forceBModel1 13
      set cg_forceBModel2 10

  other game types:
      set cg_forceBModel1 12
      set cg_forceBModel2 11



  note:  to find the model index find the entity number using 'cg_drawEntNumbers 1', then /printentitystate <ent number>,  and use the value given for 'modelindex'


* cg_proxMineTick  (play proximity mine ticking sound)
* cg_drawProxWarning*   (message about being mined and about to explode)
* /dumpents [filename] [ent numbers | 'freecam' | 'all'] [...]
     without any arguments defaults to all and filename is dump/ent
     'freecam' includes freecam, cameras, paths, etc..

  /stopdumpents

  Every line of the dump file has:

  server time, entity number, entity type, origin[0], origin[1], origin[2], angles[0], angles[1], angles[2], weapon, dead status, just teleported (includes things like respawing), legs anim number, torso anim number

  entity number -1  is freecam

  entity type:
        ET_GENERAL = 0
        ET_PLAYER = 1
        ET_ITEM = 2
        ET_MISSILE = 3   (check weapon to see what type of missile)
        ET_MOVER = 4
        ET_BEAM = 5
        ET_PORTAL = 6
        ET_SPEAKER = 7
        ET_PUSH_TRIGGER = 8
        ET_TELEPORT_TRIGGER = 9
        ET_INVISIBLE = 10
        ET_GRAPPLE = 11
        ET_TEAM = 12
        ET_EVENTS = 13

  weapon:
        WP_NONE = 0
        WP_GAUNTLET = 1
        WP_MACHINEGUN = 2
        WP_SHOTGUN = 3
        WP_GRENADE_LAUNCHER = 4
        WP_ROCKET_LAUNCHER = 5
        WP_LIGHTNING = 6
        WP_RAILGUN = 7
        WP_PLASMAGUN = 8
        WP_BFG = 9
        WP_GRAPPLING_HOOK = 10
        WP_NAILGUN = 11
        WP_PROX_LAUNCHER = 12
        WP_CHAINGUN = 13

  You can use 'fixedtime' cvar to guarantee write times:  ex: /set fixedtime 8  will write info at 125 frames per second without frame drops

* cg_zoomBroken  ql/q3 have weird zooming behavior where if you release zoom key early it wont zoom back down from the zoom fov you had reached, instead it will switch to full zoom fov and back down from there.  Can look as if zoom key is stuck with low timescales.
* cg_zoomIgnoreTimescale
* cg_zoomTime  how quickly, in milliseconds, to zoom in and out
* cg_demoStepSmoothing  (move up steps smoothly instead of stuttering up and down during demo playback the same as is done when playing the game)
* cg_stepSmoothTime  default is 100 (similar to quakelive) which will usually bounce up and down when crouching and going up steps.  quake3 default is 200 where even crouch will cause you to move up stairs in a straight vertical line
* cg_stepSmoothMaxChange
* cg_firstPersonSwitchSound  default "sound/wc/beep05"  play this sound when the view changes in a spectator demo.  Set to "" to disable.
* wolfcam_firstPersonSwitchSoundStyle  (0: don't play cg_firstPersonSwitchSound when using '/follow'  1:  only play when switching back and forth from selected player, including /follow [victim|killer]  2:  always play switch sound when view changes)
* com_execVerbose  default 0 to avoid spamming console when you execute a cfg file
* cg_noTaunt  disable 'taunt' sound from players

* r_showtris 2  will not have wallhack effect
* r_shownormals 2  will not have wallhack effect
* r_debugMarkSurface  when set to 1 will print the name of the map shader upon which impact marks are drawn
* r_ignoreNoMarks   allows marks on surfaces that have been set to not have marks on them.  Mostly for use with r_debugMarkSurface but can be used if you want to have extra impact marks (lamps, etc..).

  1:  allow extra impact marks except on water, slime, fog, and lava
  2:  allow even on liquids and fog (if possible)

* dynamically change shaders with  /remapshader <old shader> <new shader> [time offset -- usually 0]  [keepLightmap from first shader -- 0: no 1: yes (default)]

  ex:  /remapshader textures/gothic_floor/metalbridge02 textures/gothic_floor/largerblock3b4


  /listremappedshaders
  /clearremappedshader <shader name>
  /clearallremappedshaders

  also, as a convenience:  /remaplasttwoshaders  which remaps the last two uniquely named shaders

    ex:  'bind h /remaplasttwoshaders'  shoot surface with shader you want, shoot surface you want to change, hit 'h'  (maps 0 to 1 in console)

* cg_allowServerOverride to allow custom game type messages, sounds, sprites, etc...

* cl_numberPadInput  to disable number pad functions and allow them as input

* com_qlColors (1: quake live, 2: quake3)  colors used in hud and chat.  quakelive uses slightly pastel colors and replaces blue with baby-blue.

* /setcolortable <index number> r g b a
    0 black, 1 red, 2 green, 3 yellow, 4 blue, 5 cyan, 6 magenta, 7 white, 8 light grey, 9, medium grey, 10 dark grey
    r g b a  from 0.0 to 1.0

    for hud and chat colors

* cg_powerupLight  (enable or disable player glow when they have a powerup)

* raised shader 'animmap' limit from 8 to 2048, credit to Cyberstorm, suggested by KittenIgnition and mccormic

* cg_dominationPointTeamColor, cg_dominationPointTeamAlpha, cg_dominationPointEnemyColor, cg_dominationPointEnemyAlpha, cg_dominationPointNeutralColor, cg_dominationPointNeutralAlpha

* cg_helpIcon to enable/disable ingame help icons
* cg_helpIconStyle  (3: like quakelive -- wall hack effect and use cg_helpIconMinWidth and cg_helpIconMaxWidth, 2:  only wall hack effect,  1:  wall hack effect off,  0:  no icon)
* cg_helpIconMinWidth
* cg_helpIconMaxWidth

* cg_drawDominationPointStatus cg_drawDominationPointStatusX cg_drawDominationPointStatusY cg_drawDominationPointStatusFont cg_drawDominationPointStatusPointSize cg_drawDominationPointStatusScale cg_drawDominationPointStatusEnemyColor cg_drawDominationPointStatusTeamColor cg_drawDominationPointStatusBackgroundColor cg_drawDominationPointStatusAlpha cg_drawDominationPointStatusTextColor cg_drawDominationPointStatusTextAlpha cg_drawDominationPointStatusTextStyle

* cg_flagStyle  (1:  regular flags,  2:  holographic models,  3:  holographic models which can be colorized with cg_teamFlagColor, cg_enemyFlagColor, cg_neutralFlagColor)

* cg_flagTakenSound  alarm sounds for flag taken

* cg_debugImpactOrigin  to print to the console the origin of the last impact mark

* cg_drawSpeed  hud speedometer (in game units per second)
* cg_drawSpeedNoText  don't add 'UPS' text:  "400 UPS" ->  "400"
* cg_drawSpeedChangeColor to change text color based on speed

* r_fog  enable/disable drawing fog

* r_clearColor  if you use 'r_clear 1' to clear screen before drawing

* r_ignoreEntityMergable  (0:  use shader's 'entityMergable' flag,  1:  ignore shader's 'entityMergable' flag,  2:  (default) ignore shader's 'entityMergable' flag if depth buffer is being saved)

  quake3 has an optimization for sprites like smoke and blood that will lead to incorrect values if depth buffer is saved (mme_saveDepth).

* r_flares 2  enables id software code for adding flares to dynamic lights (a little buggy)

* cg_headShots  enable headshot reward sprite for servers that support it

* cg_rocketAimbot  for the rocket aimbot  (2: autoaim in game)

* cg_drawOrigin  draw origin, angles, and server time

* /killcountreset to reset kill counts (number of kills since last death),
                  kill count available for hud use with WCG_KILL_COUNT

* some dubugging and informational commands:

  /printtime  to print cgame and server time to the console
  /printdirvector
  /printlegsinfo
  /printplayerstate
  /printnextentitystate
  /printviewparms
  /printdatadir
  /listentities

  cg_debugEvents > 1 also prints entity information

* some server debugging and test commands (/devmap <map>) :

  /spawn "<pickup name" [optional amount forward]
      like /give but places the item in front of you

  /juice
      test EV_JUICED

  /view yaw pitch roll

------------------------------

automated scripting examples:  playdemolist.py and recorddemolist.py

  1) playdemolist.py

  plays all the demos in wolfcam-ql/playdemos.txt
  and then quits

  format:

  demo1.dm_73
  ctfpickup09.dm_73
  gaunt.dm_73

  2) recorddemolist.py

  plays demos from wolfcam-ql/recorddemos.txt and records portions of them and then quits, saving the avi files with the same name as the demo (blah.dm_73 -> videos/blah.avi)

  format:

  2:15 2:45 railing.dm_73
  5:55 6:17 teamkills.dm_73


  <starttime> <endtime> <demoname>

  also automatically seeks to start time, see instructions in the file

------------------------------


* r_ignorehwgamma  can be set to -1 to force hardware gamma if SDL incorrectly reports that the hardware doesn't support it

* cg_drawBBox  draw bounding box around players (not the same as hitbox)

* cg_drawTieredArmorAvailability  (same as quakelive)

* cg_racePlayerShader  (0: disable the invisible/ghost shader on other players in ql race mode, 1: enable for other players, 2: enable for other players and also use ghost shader for followed player and weapon in third person, 3: enable for other players and also use ghost shader for followed player's weapon in third person, 4:  enable for other players and also use ghost shader for followed player (but not weapon) in third person)

* cl_maxRewindBackups  Number of seek points to divide the demo into.  The higher, the more response fast forwarding and rewinding becomes.  Note that each backup point will require about 1.7MB .  The default is 12.

* cl_keepDemoFileInMemory  set to 1 can improve performance when rewinding and fastforwarding, set to 0 if you need to work with a demo file that is completed and not available to load completely at the start of demo play back (ex: streaming)

* cl_demoFileCheckSystem  check for demo file in the local file system as well as wolfcam and quake live directories.  (0:  no check,  1:  check local directory before wolfcam or quakelive directories, 2:  (default) check if not found in wolfcam or quake live directories)

  ex:  wolfcamql.exe +set cl_demoFileCheckSystem 1 +demo c:\tmp\dem6.dm_90

* cl_demoFile and cl_demoFileBaseName  have the file name for the current or last played demo file.

  ex:  to show current demo name with a keybind:
     bind F6 echopopupcvar cl_demoFileBaseName
* demo name is also shown in +info screen along with the length and progress counter
* cg_itemTimers  in world quake live item timers (0: not visible, 1: visible, 2: visible with depth hack)
* cg_itemTimersScale  size scale of in world timers
* cg_itemTimersOffset  vertical displacement of in world timers
* cg_itemTimersAlpha  transparency of in world timers

* cg_drawPowerupRespawn  ingame indicator shown 10 seconds before powerup respawns
* cg_drawPowerupRespawnScale  size of sprite
* cg_drawPowerupRespawnOffset  vertical offset of the sprite
* cg_drawPowerupRespawnAlpha  transparency of powerup indicator sprite

* cg_drawPowerupAvailable  ingame indicator icon showing available powerup
* cg_drawPowerupAvailableScale  size of sprite
* cg_drawPowerupAvailableOffset  vertical offset of the sprite
* cg_drawPowerupAvailableAlpha  transparency of sprite
* cg_drawPowerupAvailableFadeStart  distance from player at which the sprite begins fading
* cg_drawPowerupAvailableFadeEnd  distance from player at which sprite disappears

* debug command /printentitydistance   can given 2 entity numbers or just 1 to calculate the distance from current view origin
* cg_quadSoundRate  limits rate of quad sound in milliseconds for rapid fire weapons (mg, hmg, pg, cg).   Default is 1000.

* cpma pro-sound algorithm  cg_cpmaSound (0: disabled,  1: (default) use with cpma demos,  2:  use with all demos,  3:  for debugging, use with all demos and print skipped sounds)

    A sound is only played if it is in a player's potentially visible set and it is less than 1280.0f units away.  Exceptions are broadcast and speaker sounds.  Also, there is an exception for mvd spec coaches (not implemented yet).

* cg_soundPvs (1:  for ql demo protocol >= 91 which sends extra sound information only play sounds in potentially visible set, 2:  restrict sounds to pvs for all demos, 3:  for debugging, restrict sounds to pvs and print skipped sounds)

  Note that cg_cpmaSound > 1 will take precedence over this

* cg_soundBuffer (0:  disabled buffered sounds, 1:  enable buffered sounds)

* cg_drawFightMessage  to enable or disable the starting "FIGHT!" screen messages
* fs_extraGames*  similar to q3mme's fs_extraGames

  This allows searching for pk3s and files from a different directory.  The names specified are sub folders of fs_basepath and fs_homepath.

  fs_basepath is the installation directory (ex:  C:\stuff\programs\wolfcamql) and fs_homepath is the home directory where screenshots, videos, demos, etc. are stored (ex: C:\Users\user1\AppData\Roaming\wolfcamql).

  It's not a space separated list like q3mme.  The engine will search for a variable named fs_extraGames1.  If that is found it will keep checking (ex: fs_extraGames2) until it finds one that isn't defined.

  Ex:  wolfcamql +set fs_extraGames1 "Temp pk3s" +set fs_extraGames1 "myExplosions" ...

* fs_searchWorkshops (0: don't use steam workshop pk3s,  1: (default) load extra pk3s referenced in demos from steam workshop directories).  This checks both the steam directory and wolfcamql {APPLICATION DATA}/workshop.  This loads the pk3s before any others to prevent changing content.  Workshops can only adds new content.,  2:  This is like 1 but loads the pk3s after all the others (treating the workshop like a mod) so it can change the content that was loaded previously)

  Workshops referenced in demo can be checked in the +info screen and with the com_workshopids cvar.

* cl_downloadWorkshops  to auto download workshop items into wolfcamql workshop folder.  This uses steamcmd (https://developer.valvesoftware.com/wiki/SteamCMD) which needs to be in your executable search path or it can be specified with fs_steamcmd.

* /screenshotPNG for png screenshots
* r_pngZlibCompression  (0:  no compression,  1: (default) enables high speed zlib compression), 9:  best space compression, -1:  default zlib compression)

* ability to filter out reward types to disable both the announcment and the display icon:  cg_rewardCapture, cg_rewardImpressive, cg_rewardExcellent, cg_rewardHumiliation, cg_rewardDefend, cg_rewardAssist, cg_rewardComboKill, cg_rewardRampage, cg_rewardMidAir, cg_rewardRevenge, cg_rewardPerforated, cg_rewardHeadshot, cg_rewardAccuracy, cg_rewardQuadGod, cg_rewardFirstFrag, cg_rewardPerfect

* windows version can echo console messages in the command prompt if --console-output is used in the command line.  Color output can be controlled with com_ansiColor.

* --console-passive command line option to disable unix tty console

* cg_useDemoFov  protocol 91 transmits player fov values and this can be used to view the demo using the players fov or to try and detect zoom changes.  Note that zoom isn't transmitted in the demo so it is detected using a fov change.  This will lead to a problem if the player uses a config that changes fov (ex:  per weapon).  Values (0:  ignore demo fov (default),  1:  use player's fov,  2:  detect zoom changes).
* com_idleSleep (0:  full cpu usage while waiting for next frame, 1: (default) lower cpu usage if there's time available between frames, 2:  same as 1 with debugging output)
* Linux and Mac OS X console paste support
* If the first command line option doesn't begin with '-' or '+' it is automatically treated as a demo name.  This should make it easier to associate file name extensions (.dm_xx) with the wolfcamql executable.  This can be disabled with '--no-demo-arg' command line option.
* console page up/down keys scroll faster while CTRL or SHIFT keys are also pressed
* console HOME key scrolls the display to the top of the entire buffer
* cg_bleedTime to control the amount of time player bleeding is shown on screen
* com_logo to control logo cinematic when the program starts
* /cinematic_restart [all | <cinematic number>] to restart videos (applies to current cinematic video and videoMap shaders)
* /cinematiclist command to list currently loaded cinematics
* more than one videoMap shader can be played at the same time
* cl_cinematicIgnoreSeek to allow or disable seeking with videoMap shaders

* /unset command can use '*' at end of name to perform pattern matching.  Ex: /unset cg_*

* r_debugScaledImages to show non power of two scaled images
* r_scaleImagesPowerOfTwo  (0:  don't scale images for hardware that supports it, 1:  scale images to power of two dimensions, -1:  force non scaling even if hardware doesn't support it)

* cg_specOffsetQL (0:  use first person view like quake3, 1 (default):  no adjustment like quakelive, 2:  use third person offsets)

* cg_debugServerCommands (0 (default):  only print message for unknown commands, 1:  print all commands and arguments, 2:  only print message and arguments for unknown commands)

* cg_drawKeyPress  to enable drawing player key press information.  Also added WCG_PLAYER_KEY_PRESS_[FORWARD|BACK|RIGHT|LEFT|FIRE|JUMP|CROUCH] hud ownerdraws.  See ui/wckeypress.menu for an example.  '/loadmenu ui/wckeypress.menu' to test.

  Note that demos don't record key press information.  Demos do record movement direction which can be used to extract some of the values.  'right', 'left', and 'back' can be accurately extracted from the movement direction but 'forward' is an estimate based on the player's velocity.

* In Windows screen blanking is disabled.  It can be re-enabled by setting the SDL_VIDEO_ALLOW_SCREENSAVER environment variable.

* cg_useScoresUpdateTeam  hack for ql/minqlx bug not updating player config strings correctly.  Players can be shown on the wrong team and this will update the player teams from the score commands.

* r_opengl2_overbright  enables original opengl2 overbright handling
* r_allowSoftwareGLCoreContext  Mac OS X hack to disable software renderer with core context
* r_useCoreContext enables core context with opengl2
* /debugcpmamvd


----------

brugal

basturdo@yahoo.com

Copyright 2017  Angelo Cano
