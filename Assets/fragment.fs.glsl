#version 410

layout(location = 0) out vec4 fragColor;

in VertexData
{
    vec3 N; // eye space normal
	vec3 E;
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
	vec4 FragPosLightSpace;
} vertexData;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;
uniform sampler2D depthmap;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(depthmap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    vec3 L = normalize(vertexData.L);
	//vec3 N = normalize(texture(tex_normal, vertexData.texcoord).rgb * 2.0 - vec3(1.0));
	vec3 N = vec3(0,0,1);
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);

    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(depthmap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(depthmap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
    
	//return 0.0f;
    return shadow;
}

void main()
{
	vec3 lightColor = vec3(1.0);
	vec3 V = normalize(vertexData.E);                                            
    vec3 L = normalize(vertexData.L);
	vec3 N = normalize(texture(tex_normal, vertexData.texcoord).rgb * 2.0 - vec3(1.0));
	//vec3 N = vec3(0,0,1);
	vec3 H = normalize( V + L);
	vec3 texColor = texture(tex_color, vertexData.texcoord).rgb;  
	vec3 ambient = vec3(0.4) * lightColor;
    vec3 diffuse = max(dot(N, L), 0.0) * lightColor;
	vec3 specular = pow(max(dot(N, H), 0), 60) * lightColor;
	float shadow = ShadowCalculation(vertexData.FragPosLightSpace);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * texColor;  
	fragColor = vec4(lighting, 1.0);
}