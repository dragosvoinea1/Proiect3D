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

uniform float pauto;

void main()
{
  vec4 colour = vec4 (0.0, 0.0, 0.0, 1.0);
  
  vec4 MaterialColour = vec4 (0.15, 0.15, 0.95, 1.0);
  vec4 AmbientColour = vec4 (0.8, 0.8, 0.8, 1.0);

  vec3 N = normalize (frag.normal);
  vec3 L = normalize (vec3 (frag.EyeSpaceLightPosition - frag.EyeSpacePosition));
  vec3 R = normalize (reflect (L, N));
  vec3 V = normalize (vec3 (frag.EyeSpaceObjectPosition) - vec3 (0, 0, 0) );

  float Ka = 0.7;
  float Kd = 0.8;
  float Ks = 0.9;
  float n = 10.0;
  vec4 LightColour = vec4 (0.9, 0.9, 0.9, 1.0);
  vec4 HighlightColour = vec4 (0.96, 0.66, 0.15, 1.0);

  vec4 BaseWave = vec4 (0.05, 0.05, 0.3, 1.0);   
  vec4 TipWave = vec4 (0.15,0.15,0.95,1.0);

  // ambient
  colour = colour + Ka * BaseWave;
  // diffuse
  colour = colour + BaseWave * (Kd * max (0.0, dot (N, L))) * LightColour;
  // Specular
  colour = colour + (Ks * pow (max (0.0, dot (R, V)), n)) * HighlightColour;

  colour = colour + mix(BaseWave,TipWave, frag.wave_height/10);

  out_fragmentColour = colour;
}