#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;
layout(location = 3) in vec3 iv3tangent;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform mat4 um4mvit;

uniform int ocean;
uniform float water_time;

uniform mat4 lightSpaceMatrix;

out VertexData
{
    vec3 N; // eye space normal
	vec3 E;
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
	vec4 FragPosLightSpace;
} vertexData;

void main()
{
	vec3 wave = iv3vertex;
	wave.y = (sin(20.0 * wave.x + water_time/1000.0) * cos(1.5 * wave.y + water_time/1000.0)  * 100) + wave.y;
	vec4 PW = um4mv * vec4(wave, 1.0);
	vec4 P;
	if(ocean==1) P = PW;
	else P = um4mv * vec4(iv3vertex, 1.0);
	vec3 V = P.xyz;      
	
	vec4 N4 = um4mvit * vec4(iv3normal, 1.0f);
    vec3 N = normalize( N4.xyz );   
	vertexData.N = N;

	vec4 T4 = um4mv * vec4(iv3tangent, 1.0f);
    vec3 T = normalize( T4.xyz );

	vec3 B = cross(N, T); 

	mat3 TBN = transpose(mat3( T, B, N));

	vec4 L4 =  um4mvit * vec4(-160.0, 180.0, 125.0, 1.0);//vec4(-1000000.0, 3000000.0, 1000000.0, 1.0) ;
	vec3 L = normalize( L4.xyz - P.xyz);   // L - P
    vertexData.L = normalize( TBN * L); 

	V = -P.xyz;
    vertexData.E = normalize( TBN * V);

	vertexData.texcoord = iv2tex_coord;  
	gl_Position = um4p * P;

	vertexData.FragPosLightSpace = lightSpaceMatrix * vec4(iv3vertex, 1.0);
}
