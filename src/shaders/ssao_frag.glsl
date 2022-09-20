#version 450 core 

layout(location = 0) out vec4 color;

in vec2 fragTex;

uniform mat4 V_eye;
uniform mat4 P_eye;

uniform vec3 dome[64];
uniform float radius;
uniform float bias;

layout(location = 0) uniform sampler2D texpos;
layout(location = 1) uniform sampler2D texnorm;
layout(location = 2) uniform sampler2D noise;

float calcAmbientOcclusion(vec4 viewSpacePos, vec4 viewSpaceNorm) {    
    vec3 ez = viewSpaceNorm.xyz;
    vec3 ex = cross(ez, vec3(0, 1, 0));
    vec3 ey = cross(ez, ex);

    mat4 R = mat4(1.0);

    R[0] = vec4(ex, 0);
    R[1] = vec4(ey, 0);
    R[2] = vec4(ez, 0);
    R[3] = vec4(0, 0, 0, 1);

    float occlusion = 0;

    for(int i = 0; i < 64; i++)
    {
        mat4 R_z = mat4(1.0);
        float angle = texture(noise, fragTex).r;

        R_z[0] = vec4(cos(angle), sin(angle), 0, 0);
        R_z[1] = vec4(-sin(angle), cos(angle), 0, 0);
        R_z[2] = vec4(0, 0, 1, 0);
        R_z[3] = vec4(0, 0, 0, 1);
        

        vec3 rotated_dome = (R_z * vec4(dome[i], 0)).xyz;
        vec3 additional_point = (R * vec4(rotated_dome, 0)).xyz;
        vec3 samplePos = viewSpacePos.xyz + additional_point;
        

        
        vec4 offset = vec4(samplePos, 1);
        offset      = P_eye * offset;    // from view to clip-space
        offset.xyz /= offset.w;               // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        float sampleDepth = (V_eye * texture(texpos, offset.xy)).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewSpacePos.z - sampleDepth));
        occlusion       += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }


    return clamp(1.0 - (occlusion / 64.0f), 0.0, 1.0);
}

void main() {
    vec3 texturepos = texture(texpos, fragTex).rgb;
	vec3 texturenorm = normalize(texture(texnorm, fragTex).rgb);

    vec4 viewSpacePos = V_eye * vec4(texturepos, 1);
    vec4 viewSpaceNorm = normalize(V_eye * vec4(texturenorm, 0.0));

    color.rgb = vec3(calcAmbientOcclusion(viewSpacePos, viewSpaceNorm));
    //color.rgb = viewSpaceNorm.xyz;
    color.a = 1;
}