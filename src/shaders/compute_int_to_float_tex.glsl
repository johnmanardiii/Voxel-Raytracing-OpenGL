#version 450 
layout(local_size_x = 1, local_size_y = 1) in;											//local group of shaders
layout(r32ui, binding = 0) uniform uimage3D voxeltex_r;
layout(r32ui, binding = 1) uniform uimage3D voxeltex_g;
layout(r32ui, binding = 2) uniform uimage3D voxeltex_b;
layout(r32ui, binding = 3) uniform uimage3D voxeltex_a;
layout(rgba32f, binding = 4) uniform image3D voxeltex_out_float;

void main() 
{
	ivec3 voxel_coords = ivec3(gl_GlobalInvocationID.xyz);
	
	float voxel_r = imageLoad(voxeltex_r, voxel_coords).r;
	float voxel_g = imageLoad(voxeltex_g, voxel_coords).r;
	float voxel_b = imageLoad(voxeltex_b, voxel_coords).r;
	float voxel_a = imageLoad(voxeltex_a, voxel_coords).r;

	vec3 floatColor = (vec3(voxel_r, voxel_g, voxel_b)/voxel_a) / 255.0;

	float alpha = (voxel_a != 0) ? 1.0 : 0.0;

	imageStore(voxeltex_out_float, voxel_coords, vec4(floatColor, alpha));
}