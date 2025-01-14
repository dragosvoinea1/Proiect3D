// A basic vertex shader, intended to help explore light shaders. 

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
    vec4 EyeSpaceLightPosition;     // Used in later steps
    vec4 EyeSpacePosition;          // Used in later steps
    vec4 EyeSpaceObjectPosition;
    vec4 ShadowCoord;               // Used in later steps
    float wave_height;
} VertexOut; 

uniform float pauto;

// Vertex sahder code
void main()
{
    vec4    Position;
    vec3    Normal;

    Position = vec4 (VertexPosition, 1);

    // Calculate the normal matrix and apply to normal vector... 
    mat4 normalMatrix = transpose(inverse(ViewMatrix * WorldMatrix));

    Normal = VertexNormal;

    VertexOut.normal = (normalMatrix * vec4(Normal, 0)).xyz;
    VertexOut.colour = vec4(1.0,0.0,1.0,1.0);    // Testing - hard code colour to BLUE;
    VertexOut.st = VertexST;                    // Texture mapping coordinate as passed in

    // Populate  other important quantities, also in eye
    // coordinates. The position of the light. 
    VertexOut.EyeSpaceLightPosition =  ViewMatrix * WorldMatrix * LightPosition;
    VertexOut.EyeSpacePosition = ViewMatrix * WorldMatrix * Position;
    VertexOut.EyeSpaceObjectPosition = ViewMatrix * WorldMatrix * vec4 (0, 0, 0, 1);

    VertexOut.ShadowCoord = ProjectionMatrix * ShadowMatrix * WorldMatrix * Position;
    VertexOut.ShadowCoord = VertexOut.ShadowCoord / VertexOut.ShadowCoord.w;
    VertexOut.ShadowCoord = (VertexOut.ShadowCoord + vec4 (1.,1.,1.,0.)) *
                               vec4 (1./2., 1./2., 1./2., 1);

    // Populate the requried output of the Vertex Shader
	// -------------------- STYART ----------------------
	
	float auto_pos = pauto/100;
	float wave_raw = 0.2 + auto_pos;
	float wave = sin(wave_raw)*5;
	
	// 4. How about a nice scaling transformation
	float scalefactor = exp(-0.5);

	mat4 S = mat4(vec4(scalefactor, 0, 0, 0),
				  vec4(0, scalefactor, 0, 0),
				  vec4(0, 0, scalefactor, 0),
				  vec4(0, 0, 0, 1) );


    
	// TRANSFORMATION
	
    mat4 T = mat4(vec4 (1, 0, 0, 0), 
                vec4 (0, 1, 0, 0), 
                vec4 (0, 0, 1, 0), 
                vec4 (0, wave, 0, 1) ); 

    // Apply the transformation matrix (Translation in this case) 
    // to the given Vertex
    //Position = T * vec4 (VertexPosition, 1); 

	// ROTATION
	
	float angle = cos(wave_raw)*0.5; 

	mat4 Rx = mat4(vec4(cos(angle), sin(angle), 0, 0),
				  vec4(-sin(angle), cos(angle), 0, 0),
				  vec4(0, 0, 1, 0),
				  vec4(0, 0, 0, 1) );
				  
	mat4 Ry = mat4(vec4(cos(-1.2), 0, sin(-1.2), 0),
				  vec4(0, 1, 0, 0),
				  vec4(-sin(-1.2), 0, cos(-1.2), 0),
				  vec4(0, 0, 0, 1) );
				  
	mat4 Rz = mat4(vec4(1, 0, 0, 0),
				  vec4(0, cos(angle), -sin(angle), 0),
				  vec4(0, sin(angle), cos(angle), 0),
				  vec4(0, 0, 0, 1) );



	// Position = T * vec4(VertexPosition, 1);
	Position = S * T * Rx * Rz * Ry *vec4(VertexPosition, 1);
	
    gl_Position = ProjectionMatrix * ViewMatrix * WorldMatrix * Position;
}
