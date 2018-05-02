uniform sampler2D u_DiffuseMap;

varying vec2      var_Tex1;


void main()
{
	gl_FragColor = texture2D(u_DiffuseMap, var_Tex1);
}
