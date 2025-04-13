#version 330 core
in vec2 frag_UV;
out vec4 frag_COLOR;

uniform sampler2D text;
uniform vec3 textColor;

void main() {
	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, frag_UV).r);
	frag_COLOR = vec4(textColor, 1.0) * sampled;
}