#version 410 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D reflectMap;

void main()
{             
    vec3 Value = texture(reflectMap, TexCoords).rgb;
    FragColor = vec4(Value, 1.0);
}