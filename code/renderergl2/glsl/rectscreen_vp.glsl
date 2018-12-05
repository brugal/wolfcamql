
// don't include here since it will end up after extra defines added in tr_glsl.c  :  mesa reports error: #extension directive is not allowed in the middle of a shader

//#extension GL_ARB_texture_rectangle : enable

attribute vec3 attr_Position;
attribute vec4 attr_TexCoord0;

uniform mat4   u_ModelViewProjectionMatrix;
varying vec4 mvTexCoord[2];

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	mvTexCoord[0] = attr_TexCoord0;
}
