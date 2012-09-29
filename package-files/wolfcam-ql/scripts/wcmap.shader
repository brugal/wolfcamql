wc/map/custom
{
        qer_editorimage gfx/wc/custom.tga
        //surfaceparm noimpact
        //surfaceparm nolightmap
        {
                map gfx/wc/custom.tga
        }
}

wc/map/customlit
{
        qer_editorimage gfx/wc/custom.tga

        {
                //map gfx/wc/custom.tga
                map textures/base_wall/concrete_dark.tga
                rgbgen identity
        }

        {
                map $lightmap
                blendfunc filter
                rgbgen identity
        }
}
