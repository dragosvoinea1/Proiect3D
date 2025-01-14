#version 330 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexST;

uniform float a;
uniform float b;
uniform float c;

uniform vec4 LightPosition;
uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 ShadowMatrix;

out VertexData
{
    vec3 normal;
    vec4 colour;
    vec2 st;
    vec4 EyeSpaceLightPosition;
    vec4 EyeSpacePosition;     
    vec4 EyeSpaceObjectPosition;
    vec4 ShadowCoord;          
	float wave_height;	

} VertexOut; 

uniform float pauto;
uniform float ranNum;

void main()
{
    vec4    Position;
    vec3    Normal;

	float scalefactor = 20;
	mat4 S = mat4(vec4(scalefactor, 0, 0, 0), vec4(0, scalefactor, 0, 0), vec4(0, 0, scalefactor, 0), vec4(0, 0, 0, 1) );
    Position = S * vec4 (VertexPosition, 1);
	
    mat4 normalMatrix = transpose(inverse(ViewMatrix * WorldMatrix));

     Normal = VertexNormal;

    VertexOut.normal = (normalMatrix * vec4(Normal, 0)).xyz;

    VertexOut.normal = (normalMatrix * vec4(Normal, 0)).xyz;
    VertexOut.colour = vec4(1.0,0.0,1.0,1.0); 
    VertexOut.st = VertexST;                    

    VertexOut.EyeSpaceLightPosition =  ViewMatrix * WorldMatrix * LightPosition;
    VertexOut.EyeSpacePosition = ViewMatrix * WorldMatrix * Position;
    VertexOut.EyeSpaceObjectPosition = ViewMatrix * WorldMatrix * vec4 (0, 0, 0, 1);

    VertexOut.ShadowCoord = ProjectionMatrix * ShadowMatrix * WorldMatrix * Position;
    VertexOut.ShadowCoord = VertexOut.ShadowCoord / VertexOut.ShadowCoord.w;
    VertexOut.ShadowCoord = (VertexOut.ShadowCoord + vec4 (1.,1.,1.,0.)) *
                               vec4 (1./2., 1./2., 1./2., 1);

	float auto_pos = pauto/100;
	Position.y += sin(0.2*Position.x + (auto_pos))*5;
	Position.x += sin(0.1*Position.z + (auto_pos))*5;
	Position.z += sin(0.1*Position.y + (auto_pos))*5;

	float rand = sin((ranNum/10) + Position.x + Position.y + Position.z)/3;
	Position.y += rand;
	Position.x += rand*2;
	Position.z += rand*5;

	VertexOut.wave_height = sin(0.2*Position.x + (auto_pos))*5;

    gl_Position = ProjectionMatrix * ViewMatrix * WorldMatrix * Position;
}
