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
