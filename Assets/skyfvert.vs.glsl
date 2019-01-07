#version 410

layout(location = 0) in vec3 spos;

uniform mat4 smv;
uniform mat4 sp;
out vec3 TexCoords;


void main()
{
	

	gl_Position = sp * smv * vec4(spos, 1.0);
	TexCoords = spos;
	
}