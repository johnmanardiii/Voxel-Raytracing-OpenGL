#version 410 core
layout(location = 0) out vec4 color_out;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out int mask_out;

in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;

uniform sampler2D tex;
uniform float glTime;

void main()
{
	vec3 n = normalize(vertex_normal);
	vec3 lp=vec3(10,-20,-100);
	vec3 ld = normalize(vertex_pos - lp);
	float diffuse = dot(n,ld);

	vec4 tcol = texture(tex, vertex_tex);
	// get rid of the background color of the texture in tex
	tcol.a = (tcol.r + tcol.g + tcol.b) / 3.;
	tcol.a = pow(tcol.a, 3);
	if(tcol.a < .98)
	{
		discard;
	}
	color_out = tcol;
	pos_out = vec4(vertex_pos, 1);
	norm_out = vec4(vertex_normal, 1);
	mask_out = 2;
}
