#version 450 
layout(local_size_x = 1, local_size_y = 1) in;											//local group of shaders
layout(rgba16, binding = 0) uniform image2D img_input;									//input image

layout(std430, binding=1) volatile buffer shader_data
{
	int sum_luminance;
	int max_luminance;
};

float luminance(vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

void main() 
{
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	ivec2 texDim = imageSize(img_input);
	vec2 center = vec2(texDim.x/2.0f, texDim.y/2.0f);
	
	vec4 col=imageLoad(img_input, pixel_coords);

	float l = luminance(col.rgb);
	l = col.r;

	atomicMax(max_luminance, int(l * 100));

	float weight = 1.0 - (distance(pixel_coords, center) / distance(pixel_coords, vec2(0.0f, 0.0f)));

	int l_shift = int(l * 100);

	atomicAdd(sum_luminance, l_shift);

	
}