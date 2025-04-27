#version 330 core
in vec2 frag_UV;
out vec4 frag_COLOR;

uniform sampler2D image;
uniform vec4 color;
uniform bool sampleAlpha;
uniform bool hasTexture;
uniform bool dontApplyColor;

void main() {
	vec4 sampled = texture(image, frag_UV);
	if(sampleAlpha) sampled = vec4(1.0, 1.0, 1.0, texture(image, frag_UV).r);
	if(hasTexture) {
		if(!dontApplyColor) frag_COLOR = color * sampled;
		else frag_COLOR = sampled;
	}
	else frag_COLOR = color;
}