#version 310 es

precision highp int;
precision highp float;

uniform sampler2D uTexture;
uniform sampler2D uTex2D0;

out vec4 FragColor;
in vec3 gFacetNormal;
in vec3 gTriDistance;
in vec3 gPatchDistance;
in float gPrimitive;
uniform vec3 LightPosition;
uniform vec3 DiffuseMaterial;
uniform vec3 AmbientMaterial;

layout(binding = 1) buffer buffer_In {
    vec4 In[];
};

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0.0, 1.0);
    d = 1.0 - exp2(-2.0*d*d);
    return d;
}

void main()
{
    vec3 N = normalize(gFacetNormal);
    vec3 L = LightPosition;
    float df = abs(dot(N, L));
    vec3 color = AmbientMaterial + df * DiffuseMaterial + texture(uTexture, N.xy).xyz + texture(uTex2D0, N.xy).xyz;

    float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
    float d2 = min(min(gPatchDistance.x, gPatchDistance.y), gPatchDistance.z);
    color = amplify(d1, 40.0, -0.5) * amplify(d2, 60.0, -0.5) * color;

    FragColor = vec4(color, 1.0) + In[0];
}

