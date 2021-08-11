wc/defragItemShader
{
        {
                map textures/effects/quadmap2.tga
                blendfunc gl_one gl_one
                tcgen environment
        }
}

wc/poster
{
        nopicmip
        polygonOffset
        {
                map gfx/wc/poster.png
                blendfunc blend
                rgbGen vertex
                alphaGen vertex
        }
}

wc/levelShotDetail
{
        nopicmip
        {
                map gfx/wc/detail2.tga
                blendfunc GL_DST_COLOR GL_SRC_COLOR
                rgbgen identity
        }
}

// 2016-06-10 unused
/*
wc/bloodTrail {
        nopicmip
        entityMergable
        {
                clampmap gfx/damage/blood_spurt.tga
                blendfunc blend
                rgbgen vertex
                alphagen vertex
        }
}

wc/bloodTrailAlt {
        nopicmip
        entityMergable
        {
                clampmap gfx/wc/blood_spurt.tga
                blendfunc blend
                rgbgen vertex
                alphagen vertex
        }
}
*/

wc/bloodExplosionAlt {  // blood at point of impact
    nopicmip
    cull disable
    {
	animmap 5 gfx/wc/blood201.tga gfx/wc/blood202.tga gfx/wc/blood203.tga gfx/wc/blood204.tga gfx/wc/blood205.tga
        blendfunc blend
    }
}

// unused 2016-06-10
/*
wc/bloodMark {
    nopicmip
    polygonOffset
    {
        clampmap gfx/damage/blood_stain.tga
        blendFunc blend
        rgbgen identityLighting
        alphagen vertex
    }
}

wc/bloodMarkAlt {
    nopicmip
    polygonOffset
    {
        clampmap gfx/wc/blood_stain.tga
        blendFunc blend
        rgbgen identityLighting
        alphagen vertex
    }
}
*/

wc/slice5
{
	nopicmip
	novlcollapse
	{
		map gfx/2d/timer/slice_5.tga
		blendfunc blend
		rgbgen vertex
		alphagen vertex
	}
}

wc/slice5Current
{
	nopicmip
	novlcollapse
	{
		map gfx/2d/timer/slice_5.tga
		blendfunc blend
		rgbgen vertex
		alphaGen wave sin 1 .5 0 5
	}
}

mme_additiveWhite
{
        {
                map *white
                blendfunc GL_SRC_ALPHA GL_ONE
                rgbGen vertex
                alphaGen vertex
         }
}


wc/wallhackplain
{
	{
		map $whiteimage
		blendFunc GL_DST_COLOR GL_ONE
	        rgbGen entity
	}

	{
		map $whiteimage
		blendFunc GL_DST_COLOR GL_ONE
		rgbGen entity
	}
}

wc/wallhack
{
	nopicmip
	//deformVertexes wave 100 sin 3 0 0 0

	{
		map $whiteimage
		//map textures/effects/quadmap2.tga
		//map textures/effects/icemap.tga

		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA

	        rgbGen entity

		// GT0
		//alphaFunc GE128
		alphaGen entity

		//tcGen environment
		//tcmod rotate 30
                //tcmod scroll 1 .1
	}

}

wc/keyPressRight
{
        nopicmip
        nomipmaps
        {
                clampmap gfx/wc/arrow-right.png
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

wc/keyPressLeft
{
        nopicmip
        nomipmaps
        {
                clampmap gfx/wc/arrow-left.png
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

wc/keyPressForward
{
        nopicmip
        nomipmaps
        {
                clampmap gfx/wc/arrow-up.png
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

wc/keyPressBack
{
        nopicmip
        nomipmaps
        {
                clampmap gfx/wc/arrow-down.png
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

wc/keyPressMisc
{
        nopicmip
        nomipmaps
        {
                clampmap gfx/wc/circle.jpg
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

wc/hud/ntf
{
	nopicmip
	{
		map gfx/wc/ntf2.png
		blendfunc blend
		alphaFunc GE128
	}
}

wc/hud/sp
{
	nopicmip
	{
		map gfx/wc/single-player.png
		blendfunc blend
		alphaFunc GE128
	}
}
