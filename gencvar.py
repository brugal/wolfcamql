#!/usr/bin/env python
import sys, string

if (len(sys.argv) < 2):
    print "%s <cvar name>" % sys.argv[0]
    sys.exit(1)

cvarName = sys.argv[1]
#pre = "vmCvar_t"
shortName = string.lower(cvarName[len("cg_draw")]) + cvarName[len("cg_drawx"):]

for pre in ("extern vmCvar_t", "vmCvar_t"):
    print "%s %sX;" % (pre, cvarName)
    print "%s %sY;" % (pre, cvarName)
    print "%s %sAlign;" % (pre, cvarName)
    print "%s %sStyle;" % (pre, cvarName)
    print "%s %sFont;" % (pre, cvarName)
    print "%s %sPointSize;" % (pre, cvarName)
    print "%s %sScale;" % (pre, cvarName)
    print "%s %sTime;" % (pre, cvarName)
    print "%s %sColor;" % (pre, cvarName)
    print "%s %sAlpha;" % (pre, cvarName)
    print "%s %sFade;" % (pre, cvarName)
    print "%s %sFadeTime;" % (pre, cvarName)
    print
    print


print "fontInfo_t %sFont;" % shortName
print "int %sFontModificationCount;" % shortName
print
print

print "{ &%sX, \"%sX\", \"\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sY, \"%sY\", \"\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sAlign, \"%sAlign\", \"0\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sStyle, \"%sStyle\", \"0\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sFont, \"%sFont\", \"\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sPointSize, \"%sPointSize\", \"24\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sScale, \"%sScale\", \"0.25\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sTime, \"%sTime\", \"\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sColor, \"%sColor\", \"0xffffff\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sAlpha, \"%sAlpha\", \"255\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sFade, \"%sFade\", \"1\", CVAR_ARCHIVE }," % (cvarName, cvarName)
print "{ &%sFadeTime, \"%sFadeTime\", \"200\", CVAR_ARCHIVE }," % (cvarName, cvarName)

print
print



print "CG_CheckIndividualFontUpdate(&cgs.media.%sFont, %sPointSize.integer, &%sFont, &cgs.media.%sFontModificationCount);" % (shortName, cvarName, cvarName, shortName)

print
print "cgs.media.%sFontModificationCount = -100;" % shortName
print

print "vec4_t color;"
print "int x, y, w;"
#print "int scale, alpha, style, align;"
print "int style, align;"
print "float scale;"
print "fontInfo_t *font;"
print "float *fcolor;"
print
print "SC_Vec4ColorFromCvars(color, %sColor, %sAlpha);" % (cvarName, cvarName)
print "if (%sFade.integer) {" % cvarName
print "    fcolor = CG_FadeColor(xxx, %sFadeTime.integer);" % cvarName
print """
    if (!fcolor) {
        return;
    }
    color[3] -= (1.0 - fcolor[3]);
    }

if (color[3] <= 0.0) {
    return;
}

"""
print "align = %sAlign.integer;" % cvarName
print "scale = %sScale.value;" % cvarName
print "style = %sStyle.integer;" % cvarName
#print "font = &cgs.media.%sFont;" % shortName
print """if (*%sFont.string) {
    font = &cgs.media.%sFont;
} else {
    font = &cgDC.Assets.textFont;
}
""" % (cvarName, shortName)

print "x = %sX.integer;" % cvarName
print "y = %sY.integer;" % cvarName
print
print """
        s = "Waiting for players";

        w = CG_Text_Width(s, scale, 0, font);
        if (align == 1) {
            x -= w / 2;
        } else if (align == 2) {
            x -= w;
        }
        CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
"""

