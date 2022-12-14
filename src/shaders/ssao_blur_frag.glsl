#version 450 core
layout(location = 0) out vec4 color;
  
in vec2 fragTex;
  
layout(location = 0) uniform sampler2D ssaoInput;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, fragTex + offset).r;
        }
    }
    color = vec4(vec3(result / (4.0 * 4.0)), 1);
} 