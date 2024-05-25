#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUv0;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {

	vec4 texColor = texture( texSampler, inUv0 );

	fragColor = vec4( texColor.rgb * inColor, texColor.a );
}