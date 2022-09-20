#version 450 core 

layout(location = 0) out vec4 color_out;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out int mask_out;

in vec3 WorldPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 Vertex_EyeVec;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D texNormal;

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

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
    vec3 map = texture(texNormal, texcoord).xyz;
    map = map * 255./127. - 128./127.;
    mat3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * map);
}


void main()
{
	vec3 normal = normalize(fragNor);
	vec3 texturecolor = texture(tex, fragTex).rgb;
    // vec3 viewPos = normalize(Vertex_EyeVec.xyz);
	//vec3 PN = perturb_normal(normal, viewPos, fragTex);
	
	color_out.rgb = texturecolor;
	color_out.a=1;

	pos_out = vec4(WorldPos, 1);
    mask_out = 1;
	norm_out = vec4(normal, 1);     // replace with PN for normal mapped
}
