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

wc/bloodExplosion {  // blood at point of impact
    nopicmip
    cull disable
    {
        map models/weaphits/blood201.tga
        blendfunc blend
        alphagen entity
        rgbgen entity
    }
}

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
