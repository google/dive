#version 310 es
precision highp float;
precision highp int;

layout(local_size_x=5, local_size_y=6, local_size_z=7) in;

layout(binding = 1) buffer buffer_In {
    uvec4 In[];
};

layout(binding = 3) buffer buffer_In2 {
    uvec4 In2[];
};

layout(binding = 5) buffer buffer_In3 {
    uvec4 In3[];
};

layout(binding = 0) buffer buffer_Out {
    uvec4 Out[];
};

layout(binding = 2) buffer buffer_Out2 {
    uvec4 Out2[];
};

layout(binding = 4) buffer buffer_Out3 {
    uvec4 Out3[];
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
	Out[3] = Out2[2] = Out3[gl_WorkGroupID.x] = In[4] + In2[5] + In3[gl_WorkGroupID.x];
}

