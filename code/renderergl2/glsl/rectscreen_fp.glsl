
// don't include here since it well end up after extra defines added in tr_glsl.c  :  mesa reports error: #extension directive is not allowed in the middle of a shader

// #extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect backBufferTex;

void main()
{
	gl_FragColor = texture2DRect(backBufferTex, gl_TexCoord[0].xy);
}
