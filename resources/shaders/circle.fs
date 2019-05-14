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

void main(void)
{
	// draw circle
	if (colorModifier.x != -1.0) {
		if (length(ex_TexCoord - vec2(0.5)) > 0.5) discard;
		out_Color = colorModifier;
	} else { // draw arrow
		out_Color = vec4(1.0, 0.0, 0.0, 1.0) * texture(albedoTexture, ex_TexCoord);
	}
}