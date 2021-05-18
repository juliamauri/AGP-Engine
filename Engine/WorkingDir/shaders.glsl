///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef GEOMETRY_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////
"layout(location = 0) in vec3 aPos;\n"						\
"layout(location = 1) in vec3 aNormal;\n"					\
"layout(location = 2) in vec2 aTexCoord;\n"					\
"\n"														\
"out vec3 FragPos;\n"										\
"out vec2 TexCoord;\n"										\
"out vec3 Normal;\n"										\
"\n"														\
"uniform mat4 model;\n"										\
"uniform mat4 view;\n"										\
"uniform mat4 projection;\n"								\
"\n"														\
"uniform float useClipPlane;\n"								\
"uniform vec4 clip_plane;\n"								\
"\n"														\
"void main()\n"												\
"{\n"														\
"	vec4 worldPos = model * vec4(aPos, 1.0);\n"				\
"	if (useClipPlane > 0.0f)\n"								\
"		gl_ClipDistance[0] = dot(worldPos, clip_plane);\n"	\
"\n"														\
"	FragPos = worldPos.xyz;\n"								\
"	TexCoord = aTexCoord;\n"								\
"	mat3 normalMatrix = transpose(inverse(mat3(model)));\n"	\
"	Normal = normalMatrix * aNormal;\n"						\
"	gl_Position = projection * view * worldPos;\n"			\
"}\0"

#elif defined(FRAGMENT) ///////////////////////////////////////////////
"layout (location = 0) out vec3 gPosition;\n"						\
"layout (location = 1) out vec3 gNormal;\n"							\
"layout (location = 2) out vec3 gAlbedo;\n"							\
"layout (location = 3) out vec3 gSpec;\n"							\
"\n"																\
"in vec3 FragPos;\n"												\
"in vec2 TexCoord;\n"												\
"in vec3 Normal;\n"													\
"\n"																\
"uniform float useTexture;\n"										\
"uniform sampler2D tdiffuse0;\n"									\
"uniform sampler2D tspecular0;\n"									\
"uniform float shininess;\n"										\
"\n"																\
"uniform float useColor;\n"											\
"uniform vec3 cdiffuse;\n"											\
"uniform vec3 cspecular;\n"											\
"uniform float opacity;\n"											\
"\n"																\
"void main()\n"														\
"{\n"																\
"	gPosition = FragPos;\n"											\
"	gNormal = normalize(Normal);\n"									\
"\n"																\
"	if (useTexture > 0.0f && useColor > 0.0f)\n"					\
"	{\n"															\
"		gAlbedo = texture(tdiffuse0, TexCoord).rgb * cdiffuse;\n"	\
"		gSpec = vec3(texture(tspecular0, TexCoord).r, shininess,opacity);\n"\
"	}\n"															\
"	else if (useTexture > 0.0f)\n"									\
"	{\n"															\
"		gAlbedo = texture(tdiffuse0, TexCoord).rgb;\n"				\
"		gSpec = vec3(texture(tspecular0, TexCoord).r, shininess,opacity);\n"\
"	}\n"															\
"	else if (useColor > 0.0f)\n"									\
"	{\n"															\
"		gAlbedo = cdiffuse;\n"									    \
"		gSpec = vec3(cspecular.x, shininess, opacity);\n"			\
"	}\n"															\
"}\0"

#endif
#endif

#ifdef LIGHT_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////
"layout(location = 0) in vec3 aPos;\n"		\
"layout(location = 1) in vec2 aTexCoord;\n"	\
"\n"										\
"out vec2 TexCoord;\n"						\
"\n"										\
"void main()\n"								\
"{\n"										\
"	TexCoord = aTexCoord;\n"				\
"	gl_Position = vec4(aPos, 1.0);\n"		\
"}\0"

#elif defined(FRAGMENT) ///////////////////////////////////////////////
"layout (location = 4) out vec4 aRes;\n"																											\
"\n"																																				\
"in vec2 TexCoord;\n"																																\
"\n"																																				\
"struct Light {\n"																																	\
"    vec4 positionType;\n"																															\
"    vec4 directionIntensity;\n"																													\
"    vec4 diffuseSpecular;\n"																														\
"    vec4 clq; //constant linear quadratic\n"																										\
"    vec4 co; //cutoff outercutoff\n"																												\
"};\n"																																				\
"const int NR_LIGHTS = 203;\n"																														\
"uniform Light lights[NR_LIGHTS];\n"																												\
"uniform int count;\n"																															\
"uniform vec3 viewPos;\n"																															\
"\n"																																				\
"uniform sampler2D gPosition;\n"																													\
"uniform sampler2D gNormal;\n"																														\
"uniform sampler2D gAlbedo;\n"																														\
"uniform sampler2D gSpec;\n"																														\
"\n"																																				\
"vec3 calculateLight(float type, vec3 viewDir, vec3 objectPosition, vec3 lightPositon, vec3 objectNormal, vec3 objectDiffuse, vec3 lightDiffuse, float shininess, float objectSpecular, float lightSpecular, float intensity, float constant, float linear, float quadratic, vec3 direction, float cutoff, float outercutoff)\n"																																		\
"{\n"																																				\
"			vec3 lightDir = normalize(lightPositon - objectPosition);\n"																			\
"			vec3 specular = vec3(0.0, 0.0, 0.0);\n"																									\
"			vec3 res_light = vec3(0.0, 0.0, 0.0);\n"																								\
"			\n"																																		\
"			if (type == 0.0) // DIRECTIONAL_LIGHT\n"																						\
"			{\n"																																	\
"				vec3 diffuse = lightDiffuse * max(dot(objectNormal, lightDir), 0.0) * objectDiffuse;\n"													\
"				\n"																																	\
"				if (dot(objectNormal, lightDir) > 0.0)\n"																									\
"					specular = vec3(pow(max(dot(viewDir, reflect(-lightDir, objectNormal)), 0.0), shininess) * lightSpecular * objectSpecular);\n"			\
"				\n"																																	\
"				res_light = diffuse + specular;\n"																									\
"			}\n"																																	\
"			else if (type == 1.0) // POINT_LIGHT\n"																						\
"			{\n"																																	\
"				vec3 diffuse = lightDiffuse * max(dot(objectNormal, lightDir), 0.0) * objectDiffuse;\n"													\
"				\n"																																	\
"				if (dot(objectNormal, lightDir) > 0.0)\n"																									\
"					specular = vec3(pow(max(dot(viewDir, reflect(-lightDir, objectNormal)), 0.0), shininess) * lightSpecular * objectSpecular);\n"			\
"				\n"																																	\
"				float distance = length(lightPositon - objectPosition);\n"																			\
"				float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));\n"		\
"				\n"																																	\
"				diffuse *= attenuation;\n"																											\
"				specular *= attenuation;\n"																											\
"				res_light = diffuse + specular;\n"																									\
"			}\n"																																	\
"			else if (type == 2.0) // SPOT_LIGHT\n"																						\
"			{\n"																																	\
"				float theta = dot(lightDir, normalize(-direction));\n"																	\
"				if(theta > outercutoff)\n"																								\
"				{\n"																																\
"					vec3 diffuse = lightDiffuse * max(dot(objectNormal, lightDir), 0.0) * objectDiffuse;\n"												\
"					\n"																																\
"					if (dot(objectNormal, lightDir) > 0.0)\n"																								\
"						specular = vec3(pow(max(dot(viewDir, reflect(-lightDir, objectNormal)), 0.0), shininess) * lightSpecular * objectSpecular);\n"\
"					\n"																																\
"					float smoothness = clamp((theta - outercutoff) / (cutoff - outercutoff), 0.0, 1.0);\n"			\
"					\n"																																\
"					float distance = length(lightPositon - objectPosition);\n"																		\
"					float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));\n"	\
"					\n"																																\
"					diffuse *= attenuation * smoothness;\n"																							\
"					specular *= attenuation * smoothness;\n"																						\
"					res_light = diffuse + specular;\n"																								\
"				}\n"																																\
"			}\n"																																	\
"			return res_light * intensity;\n"																							\
"}\n"\
"void main()\n"																																		\
"{\n"																																				\
"	vec3 Position = texture(gPosition, TexCoord).rgb;\n"																							\
"	vec3 Normal = normalize(texture(gNormal, TexCoord).rgb);\n"																						\
"	vec3 Diffuse = texture(gAlbedo, TexCoord).rgb;\n"																								\
"	float Specular = texture(gSpec, TexCoord).r;\n"																									\
"	float shininess = texture(gSpec, TexCoord).g;\n"																								\
"	float opacity = texture(gSpec, TexCoord).b;\n"																									\
"	\n"																																				\
"   vec3 lighting = vec3(0.0, 0.0, 0.0);\n"				    																						\
"	vec3 viewDir = normalize(viewPos - Position);\n"																								\
"	\n"																																				\
"	for (int i = 0; i < count; ++i)\n"				        																					\
"   {\n"																																			\
"		lighting += calculateLight(lights[i].positionType.w, viewDir, Position, lights[i].positionType.xyz, Normal, Diffuse, lights[i].diffuseSpecular.xyz, shininess, Specular, lights[i].diffuseSpecular.w, lights[i].directionIntensity.w, lights[i].clq.x, lights[i].clq.y, lights[i].clq.z, lights[i].directionIntensity.xyz, lights[i].co.x, lights[i].co.y);\n"																							\
"   }\n"																																			\
"	aRes = vec4(lighting,opacity);\n"																												\
"}\0"

#endif
#endif

// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
