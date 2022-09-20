#version 410 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
layout (location = 3) in vec4 InstancePos;

uniform float glTime;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform float gamma;

out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;

mat4 getRotation()
{
	mat4 rot = mat4(1);
	float speed = .6f;
	// this distScale really needs to change if the values that make up the
	// distribution change.
	float distScale = length(InstancePos);
	float rotation = glTime * speed / pow(distScale, .9);
	rot[0][0] = cos(rotation);
	rot[0][2] = -sin(rotation);
	rot[2][0] = sin(rotation);
	rot[2][2] = cos(rotation);

	return rot;
}

void main()
{
	vertex_normal = vec4(M * vec4(vertNor,0.0)).xyz;
	vec4 pos = M * vec4(vertPos,1.0) + getRotation() * InstancePos;
	
	pos = V * pos;
	pos.z *= gamma;
	

	gl_Position = P * pos;
	vertex_tex = vertTex;	
}
