#version 450 core 

layout(location=0) out vec4 color;
layout(location=1) out vec4 bloom_color;
layout(location=2) out vec4 hdr_color;
layout(location=3) out vec4 raytrace_color;

in vec2 fragTex;

uniform mat4 P_light;
uniform mat4 V_light;
uniform vec3 campos;
uniform vec3 rt_dome[64];

uniform float exposure;
uniform float max_luminance;
uniform vec2 noiseScale;
uniform int num_raymarches;
uniform int num_random_samples;
uniform vec3 lp;
uniform int bloom;
uniform int tonemap;
uniform int raycast_on;

uniform bool ssaoOn;

layout(location = 0) uniform sampler2D texcol;
layout(location = 1) uniform sampler2D texpos;
layout(location = 2) uniform sampler2D texnorm;
layout(location = 3) uniform sampler2D shadowmap;
layout(binding = 4) uniform usampler3D voxoutput_alpha;
layout(binding = 5) uniform sampler3D voxoutput_float;
layout(location = 6) uniform sampler2D noiseTex;
layout(location = 7) uniform usampler2D maskTex;


// change these to uniforms later
float raymarch_dist = 0.078125;

float calcShadowFactor(vec4 lightSpacePosition) {
    vec3 shifted = (lightSpacePosition.xyz / lightSpacePosition.w + 1.0) * 0.5;

    float shadowFactor = 0;
    float bias = 0.001;
    float fragDepth = shifted.z - bias;

    if (fragDepth > 1.0) {
        return 0.0;
    }

    const int numSamples = 5;
    const ivec2 offsets[numSamples] = ivec2[](
        ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(-1, 0), ivec2(0, -1)
    );

    for (int i = 0; i < numSamples; i++) {
        if (fragDepth > textureOffset(shadowmap, shifted.xy, offsets[i]).r) {
            shadowFactor += 1;
        }
    }
    shadowFactor /= numSamples;

    return shadowFactor;
}

float luminance(vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 change_luminance(vec3 c_in, float l_out)
{
    float l_in = luminance(c_in);
    return c_in * (l_out / l_in);
}

float adjust_exposure(vec3 incolor)
{
    float L_in = luminance(incolor);
    float L_adj = L_in/(9.6 * exposure);

    return L_adj;
}

vec3 tonemapCol(vec3 incolor)
{
    float l_old = adjust_exposure(incolor);
    float numerator = l_old * (1.0f + (l_old / (max_luminance * max_luminance)));
    float l_new = numerator / (1.0f + l_old);
    return change_luminance(incolor, l_new);
}

vec3 reinhard_extended_luminance(vec3 v, float max_white_l)
{
    float l_old = luminance(v);
    float numerator = l_old * (1.0f + (l_old / (max_white_l * max_white_l)));
    float l_new = numerator / (1.0f + l_old);
    return change_luminance(v, l_new);
}

vec3 uncharted2_tonemap_partial(vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v)
{
    float exposure_bias = 2.0f;
	// function where if the max is low, then the output is high
	//float e_fact = .8 + (smoothstep(.97, 0.23, max_luminance) - .2) * .8;
    float e_fact = exposure;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias * e_fact);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

vec3 int_voxel_transform(vec3 pos)
{
    pos += vec3(10.0, 10.0, 10.0);
    pos /= 20.0;
    pos *= 256;
    ivec3 ipos = ivec3(pos);    // rounds off to the nearest texel
    return ipos / 255.0;
}

/*
    floating point for bilinear filtering on points.
*/
vec3 WorldToTex(vec3 pos)
{
    pos += vec3(10.0, 10.0, 10.0);
    pos /= 20.0;
    return pos;
}

vec3 raycast_helper(vec3 startpos, vec3 dir, vec3 normal)
{
    vec3 norm_dir = normalize(dir);
    dir = norm_dir * raymarch_dist;  // dir should be normalized
    vec3 march_pos = startpos;
    vec4 current_col;
    //int marches = 18;
    float same_dir = dot(normal, norm_dir);
    int marches = 1;
    if(same_dir < .98)
    //{
        //marches = int(min(ceil(3.0 / (dot(normal, norm_dir) + .05)), 20.0));
    //}
    //marches = 3;
    march_pos += float(marches) * dir;
    for(int i = 0; i < num_raymarches; i++)
    {
        march_pos += dir;
        marches++;
        vec3 texPosMarch = WorldToTex(march_pos);
        if(texPosMarch.x > 1.0 || texPosMarch.y > 1.0 || texPosMarch.z > 1.0 ||
        texPosMarch.x < 0.0 || texPosMarch.y < 0.0 || texPosMarch.z < 0.0)
        {
            break;
        }
        current_col = texture(voxoutput_float, texPosMarch);
        if(current_col.a > 0.02)
        {
            return current_col.rgb * pow(.98, marches);
        }
    }
    return vec3(0);
}

vec2 rand(vec3 co)
 {
     return vec2(fract(sin(dot(co ,vec3(12.9898,78.233,45.5432) )) * 43758.5453), fract(sin(dot(co ,vec3(78.233,78.233,12.9898) )) * 49823.8373));
 }

vec3 raymarch_ambient(vec3 texturepos, vec3 texturenorm)
{
    vec2 rands = rand(texturepos);
    vec3 randomVec = normalize(texture(noiseTex, rands).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - texturenorm * dot(randomVec, texturenorm));
    vec3 bitangent = cross(texturenorm, tangent);
    mat3 TBN = mat3(tangent, bitangent, texturenorm);
    vec3 raycast_color = vec3(0);
    float random_weight = 52.0 / float(num_random_samples);  // old val = 7.0, 45 looks good
    float normal_weight = .02;
    float strength = 3.0;
    random_weight *= strength;
    normal_weight *= strength;
    // get color for normal 
    // raycast_color += raycast_helper(texturepos, texturenorm, texturenorm) * normal_weight;
    int random_start = int(rands.x * 31.0);
    for(int i = 0; i < num_random_samples; i++)
    {
        vec3 random_vec = TBN * rt_dome[(i + random_start) % 30];
        //float sameness = .4 + .6 * dot(random_vec, texturenorm);
        float sameness = 1.0;
        raycast_color += raycast_helper(texturepos, random_vec, texturenorm) * random_weight * sameness;
    }
    return raycast_color;
}

void main()
{
	vec3 texturecolor = texture(texcol, fragTex).rgb;
	vec3 texturepos = texture(texpos, fragTex).rgb;
	vec3 texturenorm = normalize(texture(texnorm, fragTex).rgb);
    uint texmask = texture(maskTex, fragTex).r;

    if(texmask == 2 || texmask == 0)
    {
        hdr_color = color = vec4(texturecolor, 1);
    }
    else{
        float ssaoVal = 1.0;

        float shadow_map_depth  = texture(shadowmap, fragTex).r;

        vec4 light_screen_pos = P_light * V_light * vec4(texturepos,1);
        vec3 shifted = (light_screen_pos.xyz / light_screen_pos.w + 1.0) * 0.5;

        float shadow_factor = 1.0f - calcShadowFactor(light_screen_pos);

    
        // blinn-phong (in view-space)
        vec3 viewDir  = normalize(lp - texturepos); // viewpos is (0.0.0) in view-space
        // diffuse
        vec3 lightDir = normalize(lp - texturepos);
        vec3 diffuse = max(dot(texturenorm, lightDir), 0.0) * texturecolor * vec3(1,1,1);
        diffuse *= ssaoVal;
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(texturenorm, halfwayDir), 0.0), 8.0);
        vec3 specular = vec3(1,1,1) * spec;

        // texturenorm already has normal mapping lul
        vec3 ambient = .00 * texturecolor;
        raytrace_color = vec4(0);
        if(raycast_on != 0)
        {
            raytrace_color = vec4(raymarch_ambient(texturepos, texturenorm), 1.0);
        }
        //raytrace_color = vec4(0);

        //color.rgb = raymarch_ambient(texturepos, texturenorm);
        //color.a = 1.0;
        //return;
        //ambient = dot(ambient, ambient) > .20 ? normalize(ambient) * .20 : ambient;
        vec3 lighting  = ambient; 
        lighting += (diffuse * shadow_factor) + (specular * shadow_factor);

        float expos = sqrt(max_luminance) * 8.0;
        hdr_color = vec4(lighting, 1.0);
        if(tonemap != 0){
            color = vec4(tonemapCol(lighting), 1.0);
        }
        else{
            color = vec4(lighting, 1.0);
            }


        // color.rgb = texture(voxoutput_float, int_voxel_transform(texturepos)).rgb;
        // color.rgb = vec3(shadow_map_depth);
        // color.rgb = texturecolor;
    }
    float intensity = luminance(color.rgb);
    bloom_color  = vec4(0,0,0,0);
    if(bloom != 0){
	    if(intensity>0.98) bloom_color = vec4(color.rgb,1);
    }
}