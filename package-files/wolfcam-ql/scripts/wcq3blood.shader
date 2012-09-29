
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
