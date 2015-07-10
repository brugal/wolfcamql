adbox1x1
{
	nopicmip

	{
		map textures/ad_content/ad1x1.jpg
	}
}

adbox1x1xxxxxxx
{
        qer_editorimage textures/ad_content/ad1x1.jpg
        nopicmip

        {
                map textures/ad_content/ad1x1.jpg
        }

        {
                map $lightmap
            	rgbGen identity
                blendfunc gl_dst_color gl_zero
        }

        {
                map $lightmap
                tcgen environment
                tcmod scale .5 .5
            	rgbGen wave sin .15 0 0 0
                blendfunc add
        }
}

adbox2x1
{
	nopicmip

	{
		map textures/ad_content/ad2x1.tga
	}
}

adbox2x1_trans
{
	nopicmip
	surfaceparm nonsolid

	{
		map textures/ad_content/ad2x1.tga
                blendfunc add
		//FIXME animation
	}
}

adbox4x1
{
	nopicmip

	{
		map textures/ad_content/ad4x1.tga
	}
}

adbox8x1
{
	nopicmip

	{
		map textures/ad_content/ad8x1.tga
	}
}

adboxblack
{
	nopicmip

	{
		map gfx/colors/black.jpg
	}
}


adbox_nocull
{
	nopicmip

	{
		//map gfx/misc/bbox.tga
		map textures/ad_content/ad1x1.tga
		//map textures/ad_content/ad2x1.jpg
		//blendFunc GL_ONE GL_ONE
		//rgbGen vertex
	}
}
