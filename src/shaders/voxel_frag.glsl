#version 450 core 
out vec4 color_out;

in vec3 WorldPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 Vertex_EyeVec;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D texNormal;
layout(binding = 2, r32ui) uniform uimage3D voxoutput_red;
layout(binding = 3, r32ui) uniform uimage3D voxoutput_green;
layout(binding = 4, r32ui) uniform uimage3D voxoutput_blue;
layout(binding = 5, r32ui) uniform uimage3D voxoutput_alpha;
layout(location = 6) uniform sampler2D shadowmap;

uniform mat4 P_light;
uniform mat4 V_light;
uniform vec3 campos;
uniform vec3 lp;

// http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

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

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
    vec3 map = texture(texNormal, texcoord).xyz;
    map = map * 255./127. - 128./127.;
    mat3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * map);
}

ivec3 voxel_transform(vec3 pos)
{
    pos += vec3(10.0, 10.0, 10.0);
    pos /= 20.0;
    pos *= 256;
    ivec3 ipos = ivec3(pos);
    return ipos;
}


void main()
{
	vec3 normal = normalize(fragNor);
	vec3 texturecolor = texture(tex, fragTex).rgb;
    //vec3 viewPos = normalize(Vertex_EyeVec.xyz);
	//vec3 PN = perturb_normal(normal, viewPos, fragTex);


    float shadow_map_depth  = texture(shadowmap, fragTex).r;

    vec4 light_screen_pos = P_light * V_light * vec4(WorldPos,1);
    vec3 shifted = (light_screen_pos.xyz / light_screen_pos.w + 1.0) * 0.5;

    float shadow_factor = 1.0f - calcShadowFactor(light_screen_pos);
    
    // blinn-phong (in view-space)
    //vec3 ambient = vec3(0.08 * texturecolor); // here we add occlusion factor
    vec3 ambient = vec3(.01) * texturecolor;
    vec3 lighting  = ambient; 
    //vec3 viewDir  = normalize(lp - WorldPos); // viewpos is (0.0.0) in view-space
    // diffuse
    vec3 lightDir = normalize(lp - WorldPos);
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * texturecolor;
    // specular
    //vec3 halfwayDir = normalize(lightDir + viewDir);  
    //float spec = pow(max(dot(PN, halfwayDir), 0.0), 8.0);
    //vec3 specular = vec3(1,1,1) * spec;

    lighting += (diffuse* shadow_factor);// + (specular * shadow_factor);

    //lighting = texturecolor;
	
    ivec3 vox_pos = voxel_transform(WorldPos);
    int red = int(lighting.r * 255);
    int green = int(lighting.g * 255);
    int blue = int(lighting.b * 255);
    // add the current color (should do lighting calc here TODO)
    imageAtomicAdd(voxoutput_red, vox_pos, red);
    imageAtomicAdd(voxoutput_green, vox_pos, green);
    imageAtomicAdd(voxoutput_blue, vox_pos, blue);
    imageAtomicAdd(voxoutput_alpha, vox_pos, 1);
}
