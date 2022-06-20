#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 eyePos;
layout(location = 4) in vec3 fragPos;
layout(location = 5) in vec3 lightDir;

layout(location = 0) out vec4 outColor;

void main() {
	const vec3  diffColor = texture(texSampler, fragTexCoord).rgb;
	const vec3  specColor = vec3(1.0f, 1.0f, 1.0f);
	const float specPower = 50.0f;
	//const vec3  L = vec3(-1.0f, 0.0f, 0.0f);

	// Point light
	float decayFactor = 0.0f;
	float reductionDistance = 2.0f;
	float decay = pow((reductionDistance/length(eyePos-fragPos)), decayFactor);

	// Spot light
	float cin = 1.0f;
	float cout = 0.953f;
	float coneDim = clamp((dot(normalize(eyePos-fragPos), lightDir) - cout)/(cin-cout), 0.0f, 1.0f);

	vec3 L = normalize(eyePos - fragPos);
	vec3 N = normalize(fragNorm);
	vec3 R = -reflect(L, N);
	vec3 V = normalize(lightDir);
	
	
	// Lambert diffuse
	vec3 diffuse  = diffColor * coneDim * max(dot(N,L), 0.0f);
	// Phong specular
	//vec3 specular = diffColor * pow(max(dot(R,V), 0.0f), specPower); // Ho sostituito specColor con diffColor
	vec3 specular = vec3(0.0f);
	// Hemispheric ambient
	vec3 ambient  = 0.2f * (vec3(0.1f, 0.1f, 0.1f) * (1.0f + N.y) + vec3(0.0f, 0.0f, 0.1f) * (1.0f - N.y)) * diffColor;
	//vec3 ambient = vec3(0.0f);
	
	outColor = vec4(clamp(ambient + diffuse + specular, vec3(0.0f), vec3(1.0f)), 1.0f);
}