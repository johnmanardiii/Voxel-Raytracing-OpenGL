#version 450 core 

out vec4 color;
in vec2 fragTex;

uniform sampler2D image;
uniform int direction;
uniform float weight[10] = float[] (0.17586328091210754, 0.15958187872906446, 0.11923504813530095,
0.0733551655831621, 0.037157756871075295,0.015496663868990158,0.005320808246261766,0.0015040145363817536,
0.0003499823735729245,0.0000670412001367855);


void main()
{             
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec3 result = texture(image, fragTex).rgb * weight[0]; // current fragment's contribution
    if(direction==1)
    {
        for(int i = 1; i < 9; ++i)
        {
            result += texture(image, fragTex + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, fragTex - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 9; ++i)
        {
            result += texture(image, fragTex + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, fragTex - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    color = vec4(result, 1.0);
	
}
