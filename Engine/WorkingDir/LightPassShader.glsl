///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef LIGHT_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
	TexCoord = aTexCoord;
	gl_Position = vec4(aPos, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////
layout (location = 4) out vec4 aRes;

in vec2 TexCoord;

struct Light {
    vec4 positionType;
    vec4 directionIntensity;
    vec4 diffuseSpecular;
    vec4 clq; //constant linear quadratic
    vec4 co; //cutoff outercutoff
};
const int NR_LIGHTS = 203;
uniform Light lights[NR_LIGHTS];
uniform int count;
uniform vec3 viewPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gSpec;

vec3 calculateLight(float type, vec3 viewDir, vec3 objectPosition, vec3 lightPositon, vec3 objectNormal, vec3 objectDiffuse, vec3 lightDiffuse, float shininess, float objectSpecular, float lightSpecular, float intensity, float constant, float linear, float quadratic, vec3 direction, float cutoff, float outercutoff)
{
			vec3 lightDir = normalize(lightPositon - objectPosition);
			vec3 specular = vec3(0.0, 0.0, 0.0);
			vec3 res_light = vec3(0.0, 0.0, 0.0);

			if (type == 0.0) // DIRECTIONAL_LIGHT
			{
				vec3 diffuse = lightDiffuse * max(dot(objectNormal, lightDir), 0.0) * objectDiffuse;

				if (dot(objectNormal, lightDir) > 0.0)
					specular = vec3(pow(max(dot(viewDir, reflect(-lightDir, objectNormal)), 0.0), shininess) * lightSpecular * objectSpecular);

				res_light = diffuse + specular;
			}
			else if (type == 1.0) // POINT_LIGHT
			{
				vec3 diffuse = lightDiffuse * max(dot(objectNormal, lightDir), 0.0) * objectDiffuse;
				
				if (dot(objectNormal, lightDir) > 0.0)
					specular = vec3(pow(max(dot(viewDir, reflect(-lightDir, objectNormal)), 0.0), shininess) * lightSpecular * objectSpecular);
				
				float distance = length(lightPositon - objectPosition);
				float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
				
				diffuse *= attenuation;
				specular *= attenuation;
				res_light = diffuse + specular;
			}
			else if (type == 2.0) // SPOT_LIGHT
			{
				float theta = dot(lightDir, normalize(-direction));
				if(theta > outercutoff)
				{
					vec3 diffuse = lightDiffuse * max(dot(objectNormal, lightDir), 0.0) * objectDiffuse;
					
					if (dot(objectNormal, lightDir) > 0.0)
						specular = vec3(pow(max(dot(viewDir, reflect(-lightDir, objectNormal)), 0.0), shininess) * lightSpecular * objectSpecular);
					
					float smoothness = clamp((theta - outercutoff) / (cutoff - outercutoff), 0.0, 1.0);
					
					float distance = length(lightPositon - objectPosition);
					float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
					
					diffuse *= attenuation * smoothness;
					specular *= attenuation * smoothness;
					res_light = diffuse + specular;
				}
			}
			return res_light * intensity;
}
void main()
{
	vec3 Position = texture(gPosition, TexCoord).rgb;
	vec3 Normal = normalize(texture(gNormal, TexCoord).rgb);
	vec3 Diffuse = texture(gAlbedo, TexCoord).rgb;
	float Specular = texture(gSpec, TexCoord).r;
	float shininess = texture(gSpec, TexCoord).g;
	float opacity = texture(gSpec, TexCoord).b;

    vec3 lighting = vec3(0.0, 0.0, 0.0);
	vec3 viewDir = normalize(viewPos - Position);

	for (int i = 0; i < count; ++i)
   {
		lighting += calculateLight(lights[i].positionType.w, viewDir, Position, lights[i].positionType.xyz, Normal, Diffuse, lights[i].diffuseSpecular.xyz, shininess, Specular, lights[i].diffuseSpecular.w, lights[i].directionIntensity.w, lights[i].clq.x, lights[i].clq.y, lights[i].clq.z, lights[i].directionIntensity.xyz, lights[i].co.x, lights[i].co.y);
   }
	aRes = vec4(lighting,opacity);
}

#endif
#endif