#version 330  

precision highp float;

in VertexData
{
    vec3 normal;
    vec4 colour;
    vec2 st;
    vec4 EyeSpaceLightPosition;     
    vec4 EyeSpacePosition;          
    vec4 EyeSpaceObjectPosition;
    vec4 ShadowCoord;               
    float wave_height;
} frag; 


out vec4 out_fragmentColour;

void main()
{
  vec4 colour = vec4 (0.0, 0.0, 0.0, 1.0);

  vec4 MaterialColour = vec4 (0.52, 0.37, 0.26, 1.0);
  vec4 AmbientColour = vec4 (0.8, 0.8, 0.8, 1.0);

  vec3 N = normalize (frag.normal);
  vec3 L = normalize (vec3 (frag.EyeSpaceLightPosition - frag.EyeSpacePosition));
  vec3 R = normalize (reflect (L, N));
  vec3 V = normalize (vec3 (frag.EyeSpaceObjectPosition) - vec3 (0, 0, 0) );

  float Ka = 0.7; 
  float Kd = 0.7; 
  float Ks = 0.0; 
  float n = 10.0;
  vec4 LightColour = vec4 (0.2, 0.8, 0.0, 1.0);
  vec4 HighlightColour = vec4 (1.0, 1.0, 1.0, 1.0);

  // ambient
  colour = colour + Ka * MaterialColour;
  // diffuse
  colour = colour + MaterialColour * (Kd * max (0.0, dot (N, L))) * LightColour;
  // Specular
  colour = colour + (Ks * pow (max (0.0, dot (R, V)), n)) * HighlightColour;

  out_fragmentColour = colour;
}