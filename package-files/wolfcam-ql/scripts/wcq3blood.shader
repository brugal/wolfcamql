
q3bloodTrail
{
	nopicmip
	entityMergable
	{
		clampmap gfx/damageq3/blood_spurt.tga
  		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA

                rgbGen vertex
                alphaGen vertex
        }
}

q3bloodMark
{
	nopicmip
	polygonOffset
	{
		clampmap gfx/damageq3/blood_stain.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA

		rgbGen identityLighting
		alphaGen vertex
	}
}

q3bloodExplosion {  // blood at point of impact
    nopicmip
    cull disable
    {
	animmap 5 gfx/damageq3/blood201.tga gfx/damageq3/blood202.tga gfx/damageq3/blood203.tga gfx/damageq3/blood204.tga gfx/damageq3/blood205.tga
        blendfunc blend
    }
}

