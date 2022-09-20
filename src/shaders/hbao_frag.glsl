#version 450 core 

layout(location = 0) out vec4 color;

in vec2 fragTex;

uniform mat4 V_eye;
uniform mat4 P_eye;
uniform float bias;


layout(location = 0) uniform sampler2D texpos;
layout(location = 1) uniform sampler2D texnorm;
layout(location = 2) uniform sampler2D noise;

vec3 resampleFromPoint(vec3 samplePoint)
{
    vec4 offset = vec4(samplePoint, 1);
    offset      = P_eye * offset;    // from view to clip-space
    offset.xyz /= offset.w;               // perspective divide
    offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

    return (V_eye * texture(texpos, offset.xy)).xyz;
}


vec3 raymarch(vec3 startPos, vec3 dir)
{
    vec3 currPos = startPos;
    vec3 currDir = dir;
    int iterations = 75;
    float stepSize = 0.00125;

    for(int i = 0; i < iterations; i++)
    {
        currPos = currPos + (currDir * stepSize);
        
        vec3 samplePoint = resampleFromPoint(currPos);

        if(samplePoint.z >= currPos.z + (bias * 3) && distance(samplePoint, startPos) <= 0.25)
        {
            currDir = normalize(samplePoint - startPos);
        }

    }
    return currDir;
}


float calcAmbientOcclusion(vec4 viewSpacePos, vec4 viewSpaceNorm) {    
    float occlusion = 0;
    vec3 normal = viewSpaceNorm.xyz;

    vec3 randomVec = texture(noise, fragTex).rgb;
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);

    vec3 dir[4] = {tangent, -1.0 * tangent, bitangent, -1.0 * bitangent};
    vec3 finalRays[4];

    for(int i = 0; i < 4; i++)
    {
        finalRays[i] = raymarch(viewSpacePos.xyz, normalize(dir[i]));
        occlusion += dot(finalRays[i], normalize(dir[i]));
    }

    occlusion /= 4.0f;
    occlusion = pow(occlusion, 0.5);
    return clamp(occlusion, 0.0, 1.0);
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