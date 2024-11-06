#version 450

struct Particle {
	vec2 position;
	vec2 velocity;
};

layout (location = 0) in vec3 o_colour;
layout (location = 1) in vec2 o_texCoord;

layout (location = 0) out vec4 fragColour;

layout (binding = 0) uniform ParameterUBO {
	float time;
} ubo;

layout (binding = 1) uniform ParameterUBO2 {
	float otherData;
} ubo2;

layout (set = 0, binding = 2) readonly buffer ParticleSSBO {
    Particle particles[];
};

layout (set = 0, binding = 3) uniform sampler2D u_texture;

void main()
{
	vec3 col = texture(u_texture, o_texCoord).rgb;

	col *= o_colour;

	col *= ubo.time * ubo2.otherData;

	fragColour = vec4(col, 1.0);
}
