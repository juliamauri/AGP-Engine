///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef GEOMETRY_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 worldPos = model * vec4(aPos, 1.0);

	FragPos = worldPos.xyz;
	TexCoord = aTexCoord;
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	Normal = normalMatrix * aNormal;
	gl_Position = projection * view * worldPos;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gSpec;

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

uniform float useTexture;
uniform sampler2D tdiffuse;
uniform sampler2D tspecular;

uniform float useColor;
uniform vec3 albedo;
uniform vec3 emissive;
uniform float smoothness;

void main()
{
	gPosition = FragPos;
	gNormal = normalize(Normal);

	if (useTexture > 0.0f && useColor > 0.0f)
	{
		gAlbedo = texture(tdiffuse, TexCoord).rgb * albedo;
		gSpec = vec3(texture(tspecular, TexCoord).r, smoothness,0.0);
	}
	else if (useTexture > 0.0f)
	{
		gAlbedo = texture(tdiffuse, TexCoord).rgb;
		gSpec = vec3(texture(tspecular, TexCoord).r, smoothness,0.0);
	}
	else if (useColor > 0.0f)
	{
		gAlbedo = albedo;
		gSpec = vec3(0.0, smoothness, 0.0);
	}
}

#endif
#endif