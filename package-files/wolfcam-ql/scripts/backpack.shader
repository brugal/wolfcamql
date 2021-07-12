
icons/icon_backpack
{
	nopicmip
	{
		map icons/icon_backpack.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

models/backpack
{
	{
		map textures/sfx/specular.tga
		blendfunc GL_ONE GL_ZERO
		tcGen environment
	}
	{
		map models/backpack.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightningDiffuse
	}
}
