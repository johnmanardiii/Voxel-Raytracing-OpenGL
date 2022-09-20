#version 410 core
layout (location = 0) out vec4 color;
layout (location = 1) out vec4 BrightColor;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 fragTan;
in vec3 fragBinorm;
uniform vec3 campos;

uniform sampler2D tex;
uniform sampler2D normalTex;

vec3 CalcBumpedNormal()
{
    vec3 Normal = normalize(vertex_normal);
    vec3 Tangent = normalize(fragTan);
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    vec3 Bitangent = normalize(fragBinorm);
    vec3 BumpMapNormal = texture(normalTex, vertex_tex).xyz;
    BumpMapNormal = 2.0 * BumpMapNormal - vec3(1.0, 1.0, 1.0);
    vec3 NewNormal;
    mat3 TBN = mat3(Tangent, Bitangent, Normal);
    NewNormal = TBN * BumpMapNormal;
    NewNormal = normalize(NewNormal);
    return NewNormal;
}

void main()
{
    vec3 frag_norm = normalize(vertex_normal);

    vec3 texture_normal = texture(normalTex, vertex_tex).rgb;
    texture_normal = (texture_normal - vec3(.5)) * 2.0f;

    // calculate normal from bump map
    vec3 bumpnormal = (texture_normal.x * fragTan) + (texture_normal.y * fragBinorm) + (texture_normal.z * frag_norm);
    bumpnormal = normalize(bumpnormal);
    bumpnormal = normalize(vertex_normal);
    bumpnormal = CalcBumpedNormal();

   vec3 lightpos  = vec3(50, 100, 50);
   vec3 lightdir  = normalize(lightpos - vertex_pos);
   vec3 camdir    = normalize(campos - vertex_pos);

   float diffuse_fact = clamp(dot(lightdir, bumpnormal), 0, 1);
    diffuse_fact = clamp(diffuse_fact + .7, 0, 1);

   vec3 h = normalize(camdir + lightdir);
   float spec_fact = pow(dot(h, bumpnormal), 35);
   spec_fact = clamp(spec_fact, 0, 1);

   //vec4 tcol = texture(tex, vertex_tex)
   //color = tcol;

   color.rgb = texture(tex, vertex_tex).rgb * diffuse_fact;
   // if the color of the texture at the moment isn't very white, we want more reflection (hack for marios skin in absence
   // of a specular map)
   float tex_avg = (color.r + color.b + color.g) / 3.0f;
   color.rgb += vec3(.17) * (spec_fact * (1 - (tex_avg / 2)));
   color.a=1;


   float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > .7 && (tex_avg > .75f))
        BrightColor = vec4(color.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}