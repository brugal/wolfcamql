
// don't include here since it will end up after extra defines added in tr_glsl.c  :  mesa reports error: #extension directive is not allowed in the middle of a shader

// #extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect backBufferTex;

varying vec4 mvTexCoord[2];

void main()
{
	gl_FragColor = texture2DRect(backBufferTex, mvTexCoord[0].xy);
}
