#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;
layout(location = 2) in vec3 vertNor;
layout(location = 3) in ivec4 BoneIDs;
layout(location = 4) in vec4 Weights;
layout(location = 5) in vec3 vertTan;
layout(location = 6) in vec3 vertBinorm;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

const int MAX_BONES = 100;
uniform mat4 gBones[MAX_BONES];

out vec3 fragNor;
out vec2 fragTex;
out vec3 WorldPos;
out vec4 Vertex_EyeVec;

void main()
{
	mat4 BoneTransform = gBones[BoneIDs[0]] * Weights[0];
	BoneTransform     += gBones[BoneIDs[1]] * Weights[1];
	BoneTransform     += gBones[BoneIDs[2]] * Weights[2];
	BoneTransform     += gBones[BoneIDs[3]] * Weights[3];

	vec4 tpos =  M * BoneTransform* vec4(vertPos, 1.0);
	WorldPos = tpos.xyz;
	gl_Position = P * V * tpos;

	fragNor = vec4(M *BoneTransform * vec4(vertNor,0.0)).xyz;
	fragTex = vertTex;
	vec4 view_vertex = V * M * vec4(vertPos, 1.0);
	Vertex_EyeVec = -view_vertex;
}
