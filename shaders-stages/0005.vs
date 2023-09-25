#version 310 es

uniform sampler2D uTexture;
uniform sampler2D uTex2D0;
uniform vec4 uVec4;
in vec4 in_position;
out vec3 vPosition;

layout(binding = 1) buffer buffer_In {
    vec4 In[];
};

void main()
{
    vPosition = In[0].xyz + in_position.xyz + texture(uTexture, in_position.xy).xyz + uVec4.xyz + texture(uTex2D0, in_position.xy).xyz;
}

