#version 450 core 

out vec4 color;

in vec3 WorldPos;
in vec2 fragTex;
in vec3 fragNor;

uniform vec3 campos;


layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;


void main()
{
	vec3 normal = normalize(fragNor);
	vec3 texturecolor = texture(tex, fragTex).rgb;
	
	
	
	//diffuse light
	vec3 lp = vec3(100,100,100);

	vec3 ld = normalize(lp - WorldPos);

	float diffuse = dot(ld,normal);	

	float light = clamp(diffuse,0,1);

	//specular light
	vec3 camvec = normalize(campos - WorldPos);
	vec3 h = normalize(camvec+ld);


	float spec = pow(dot(h,normal),50);

	spec = clamp(spec,0,1);

	color = vec4(texturecolor * (light + spec), 1);
}
