#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 eyePos;
layout(location = 4) in vec3 fragPos;
layout(location = 5) in vec3 lightDir;
layout(location = 6) in vec3 refl;

layout(location = 0) out vec4 outColor;

void main() {
	const vec3  diffColor = texture(texSampler, fragTexCoord).rgb;
	const vec3  specColor = vec3(1.0f, 1.0f, 1.0f); // white torch color
	const float specPower = 200.0f;

	float decayFactor = 1.5f;
	float reductionDistance = 2.0f;
	float decay = pow((reductionDistance/length(eyePos-fragPos)), decayFactor);

	// Spot light
	float cin = 0.98f;
	float cout = 0.94f;
	float coneDim = clamp (decay * clamp((dot(normalize(eyePos-fragPos), lightDir) - cout)/(cin-cout), 0.0f, 1.0f), 0.0f, 1.0f);

	vec3 L = normalize(eyePos - fragPos);
	vec3 N = normalize(fragNorm);
	vec3 R = -reflect(L, N);
	vec3 V = normalize(eyePos - fragPos);
	
	// Lambert diffuse
	vec3 diffuse  = diffColor * coneDim * max(dot(N,L), 0.0f);
    
	// Phong specular
	vec3 specular;
    // apply specular only on objects with refl.x > 0.5
	if (refl.x > 0.5f) {
		specular = specColor * pow(max(dot(R,V), 0.0f), specPower) * coneDim;
	} else {
		specular = vec3(0.0f);
	}
    
    vec3 AmbColor = vec3(0.05, 0.05, 0.05);
	vec3 ambient = AmbColor * diffColor;
	
	outColor = vec4(clamp(ambient + diffuse + specular, vec3(0.0f), vec3(1.0f)), 1.0f);
}
