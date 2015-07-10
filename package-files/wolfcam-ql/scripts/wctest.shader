
wcwhite
{
        {
                map *white
                blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

gfx/wc/openarenachars
{
        nopicmip
        nomipmaps
        {
                map gfx/wc/openarenachars.png
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

wc/console
{
        nomipmaps
        nopicmip
        {
                map textures/sfx2/screen01.tga
                blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                tcMod scroll 7.1 0.2
                tcmod scale 0.8 1.0
                rgbgen vertex
        }

        {
                map textures/effects2/console01.tga
                blendFunc add
                tcMod scroll -0.01 -0.02
                tcmod scale 0.02 0.01
                tcmod rotate 3
                rgbgen vertex
        }

}

wc/friend
{
        nomipmaps
        nopicmip
        {
                map sprites/friend.tga
		blendfunc blend
		rgbgen vertex
        }
}

wc/foe
{
        nomipmaps
        nopicmip
        {
                map sprites/foe.tga
                blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        }
}

wc/self
{
        nomipmaps
        nopicmip
        {
                map gfx/wc/self.tga
                blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        }
}

wc/tracer
{
        nopicmip
        cull none

        {
                map gfx/wc/tracer.tga
                blendFunc GL_ONE GL_ONE
                rgbGen entity
        }
}

wc/tracerHighlight
{
        nopicmip
        cull none

        {
                map gfx/wc/tracer.tga
                blendFunc GL_ONE GL_ONE
                rgbGen entity
        }
        {
                map gfx/wc/tracer.tga
                blendFunc GL_ONE GL_ZERO
                alphaFunc LT128
        }
}

xxgibs
{
        nopicmip
               cull none
               //deformVertexes wave 10 sin 1 5.6 1 .4
               deformVertexes bulge 0.4 0.4 5
        {
                map textures/base_wall/comp3textb.tga
                blendfunc add
                rgbGen Wave sin .4 0 0 0
                tcmod scroll 3 3
        }
         {
                //map models/gibs/gibs.tga
		//map models/gibs/sphere.png
		//map textures/common/white.png
		map gfx/damage/blood_screen.png

                //blendfunc gl_src_alpha gl_one_minus_src_alpha
                //alphaFunc GE128
                blendFunc add
		//rgbGen entity
		//rgbGen identity
        }

        {
                map textures/base_wall/comp3text.tga
                blendfunc add
               // rgbGen Wave sin .5 0 0 0
                tcmod scroll 1 1
        }


}


xgibs
{
	nopicmip
	cull none
	//tesssize 2
	{
		map gfx/misc/tracer2.jpg
		//map gfx/misc/tracer8x8.jpg
		//map gfx/misc/tracer4x4.jpg

		//blendfunc add
		//blendFunc GL_ONE GL_ONE
		//blendFunc gl_dst_color gl_zero
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//blendFunc blend
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR

	}
}

wc/wcrocketaim
{
        nopicmip
        {
                map gfx/wc/wcrocket.png
                blendfunc blend
        }
}
