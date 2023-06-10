#version 450

layout(location = 0) in vec2 coord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;

void main()
{
    outColor = texture(image, coord);
    //outColor = vec4(1, 0, 1, 1);
}