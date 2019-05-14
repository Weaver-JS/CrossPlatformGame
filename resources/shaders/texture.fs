#version 420

// this come from the vertex shader
in  vec4 ex_Color;
in  vec2 ex_TexCoord;

layout(binding = 0) uniform sampler2D albedoTexture; // read from the texture slot 0

layout(location = 0) out vec4 out_Color; // write to the back buffer 0

layout (std140, binding = 0) uniform InstanceData // has to be equal to the one in the vertex shader
{
	mat4 modelMatrix;
	vec4 colorModifier;
};

uniform float radius;

void main(void)
{
	out_Color = texture(albedoTexture, ex_TexCoord) * colorModifier;
	if (out_Color.a == 0.0) discard;
}