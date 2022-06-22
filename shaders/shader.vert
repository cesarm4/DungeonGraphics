#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 eyePos;
	vec3 lightDir;
	vec3 refl;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragViewDir;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 eyePos;
layout(location = 4) out vec3 fragPos;
layout(location = 5) out vec3 lightDir;
layout(location = 6) out vec3 refl;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
	fragPos = (ubo.model * vec4(pos,  1.0)).xyz;
	fragViewDir  = (ubo.view[3]).xyz - fragPos;
	fragNorm     = (ubo.model * vec4(norm, 0.0)).xyz;
	fragTexCoord = texCoord;
	eyePos = ubo.eyePos;
	lightDir = ubo.lightDir;
	refl = ubo.refl;
}