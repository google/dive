#version 310 es
precision highp float;
precision highp int;

layout(local_size_x=5, local_size_y=6, local_size_z=7) in;

layout(binding = 0) buffer buffer_In {
    uvec3 In[];
};

layout(binding = 1) buffer buffer_Out {
    uvec3 Out[];
};

layout(rgba8) readonly uniform image2D src0;
layout(rgba8) readonly uniform image2D src1;
uniform sampler2D uTex2D0;

uniform uint max;
uniform mat3 uMat[16];
uniform mat3 uMat0;
uniform mat3 uMat1;
uniform mat3 uMat2;
uniform mat3 uMat3;

void main(void) {
    uint index = gl_LocalInvocationIndex;
    vec3 tmp0, tmp1;
    vec3 tmp2, tmp3;

    if (index > max)
        return;
    
    tmp0 = vec3(gl_GlobalInvocationID.xyz);
    tmp1 = tmp0 * texture(uTex2D0, tmp0.xy).xyz;
    tmp2 = (tmp0 + 1.0) * (tmp1 * texture(uTex2D0, tmp1.xy).xyz);
    tmp3 = (tmp0 + 1.1) * (tmp1 + 1.3) * (tmp2 * texture(uTex2D0, tmp2.xy).xyz);

    Out[index] = In[index] + uvec3(uMat3 * tmp3);
}

