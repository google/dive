#version 310 es
//#extension GL_OES_geometry_shader: enable

uniform sampler2D uTexture;
uniform sampler2D uTex2D0;
uniform mat4 Modelview;
uniform mat3 NormalMatrix;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 tePosition[3];
in vec3 tePatchDistance[3];
out vec3 gFacetNormal;
out vec3 gPatchDistance;
out vec3 gTriDistance;

layout(binding = 1) buffer buffer_In {
    vec4 In[];
};

void main()
{
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = NormalMatrix * normalize(cross(A, B)) + texture(uTexture, A.xy).xyz + texture(uTex2D0, A.xy).xyz;
    
    gPatchDistance = tePatchDistance[0];
    gTriDistance = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position + In[0]; EmitVertex();

    gPatchDistance = tePatchDistance[1];
    gTriDistance = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[2];
    gTriDistance = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}

