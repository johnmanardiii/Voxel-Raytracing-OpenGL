#version 450 core 

layout(location = 0) out vec4 color;

in vec2 fragTex;

layout(location = 0) uniform sampler2D texcol;
layout(location = 1) uniform sampler2D texbloom;
layout(location = 2) uniform sampler2D texraytrace;
layout(location = 3) uniform sampler2D objectcolors;


void main()
{
	vec3 texturecolor = texture(texcol, fragTex).rgb;
	vec4 bloomcolor = texture(texbloom, fragTex);
	vec3 raytracecolor = texture(texraytrace, fragTex).rgb;
	vec3 objectcolor = texture(objectcolors, fragTex).rgb;
	bloomcolor.a = .5;

	//color.rgb = texturecolor + bloomcolor.rgb * bloomcolor.a;
	color.rgb = texturecolor + (objectcolor * raytracecolor) + bloomcolor.rgb * bloomcolor.a;
	//color.rgb = bloomcolor.rgb;
	//color.rgb = raytracecolor;
	//color.rgb = texturecolor + (objectcolor * raytracecolor);
	//color.rgb = texturecolor;
	color.a=1;	
}