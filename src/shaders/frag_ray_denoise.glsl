#version 450 core 

out vec4 color;
in vec2 fragTex;

layout(location=0) uniform sampler2D raytrace_color;
layout(location=1) uniform sampler2D postex;
layout(location=2) uniform sampler2D normtex;

uniform int direction;

uniform mat4 V_eye;

uniform float weight[10] = float[] (0.17586328091210754, 0.15958187872906446, 0.11923504813530095,
0.0733551655831621, 0.037157756871075295,0.015496663868990158,0.005320808246261766,0.0015040145363817536,
0.0003499823735729245,0.0000670412001367855);

bool check_reqs(vec2 currentPixelCoords, vec2 nextPixelCoords)
{
    vec3 selfWorldPos = texture(postex, currentPixelCoords, 1).rgb;
    vec4 viewSpacePos = V_eye * vec4(selfWorldPos, 0);
    vec4 viewSpaceNorm = normalize(V_eye * vec4(normalize(texture(normtex, currentPixelCoords, 1).rgb), 0.0));

    vec3 nextWorldPos = texture(postex, nextPixelCoords, 1).rgb;
    vec4 nextViewSpacePos = V_eye * vec4(nextWorldPos, 1);
    vec4 nextViewSpaceNorm = normalize(V_eye * vec4(normalize(texture(normtex, nextPixelCoords, 1).rgb), 0.0));

    // compare view space depths with slight bias
    if(dot(viewSpaceNorm, nextViewSpaceNorm) < 0.68 || abs(viewSpacePos.z - nextViewSpacePos.z) > 20.0)
    {
        return false;
    }
    else
    {
        return true;
    }
}


void main()
{             
    vec2 tex_offset = 1.0 / textureSize(raytrace_color, 1); // gets size of single texel
    vec3 result = texture(raytrace_color, fragTex, 1).rgb * weight[0]; // current fragment's contribution
    float sum_weight = weight[0];
    if(direction==1)
    {
        vec2 current_tex = fragTex;
        for(int i = 1; i < 10; ++i)
        {
            vec2 nextPixelSamplePos = fragTex + vec2(tex_offset.x * i, 0.0);
            
            if(check_reqs(current_tex, nextPixelSamplePos))
            {
                result += texture(raytrace_color, nextPixelSamplePos, 1).rgb * weight[i];
                sum_weight += weight[i];
                current_tex = nextPixelSamplePos;
            }
            else
                break;
            
        }
        current_tex = fragTex;
        for(int i = 1; i < 10; ++i)
        {
            vec2 nextPixelSamplePos = fragTex - vec2(tex_offset.x * i, 0.0);
            if(check_reqs(current_tex, nextPixelSamplePos))
            {
                result += texture(raytrace_color, nextPixelSamplePos, 1).rgb * weight[i];
                sum_weight += weight[i];
                current_tex = nextPixelSamplePos;
            }
            else
                break;
        }
    }
    else
    {
        vec2 current_tex = fragTex;
        for(int i = 1; i < 10; ++i)
        {
            vec2 nextPixelSamplePos = fragTex + vec2(0.0, tex_offset.y * i);
            if(check_reqs(current_tex, nextPixelSamplePos))
            {
                result += texture(raytrace_color, nextPixelSamplePos, 1).rgb * weight[i];
                sum_weight += weight[i];
                current_tex = nextPixelSamplePos;
            }
            else
                break;
            
        }
        current_tex = fragTex;
        for(int i = 1; i < 10; ++i)
        {
            vec2 nextPixelSamplePos = fragTex - vec2(0.0, tex_offset.y * i);
            if(check_reqs(current_tex, nextPixelSamplePos))
            {
                result += texture(raytrace_color, nextPixelSamplePos, 1).rgb * weight[i];
                sum_weight += weight[i];
                current_tex = nextPixelSamplePos;
            }
            else
                break;
        }
    }
    result *= 1.0 / sum_weight;
    color = vec4(result, 1.0);
	//color = texture(raytrace_color, fragTex);
}
