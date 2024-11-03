#version 450

layout(set = 0, binding = 0) uniform sampler2D u_texture;

layout (location = 0) in vec3 o_colour;
layout (location = 1) in vec2 o_texCoord;

layout (location = 0) out vec4 fragColour;

void main()
{
	vec3 col = texture(u_texture, o_texCoord).rgb;

	col *= o_colour;
	fragColour = vec4(col, 1.0);
}