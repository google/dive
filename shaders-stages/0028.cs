#version 310 es
// compare to 0017.cs

precision highp float;
precision highp int;

layout(local_size_x=5, local_size_y=6, local_size_z=7) in;

layout(binding = 1) buffer buffer_In {
    uvec2 In[];
};

layout(binding = 0) buffer buffer_Out {
    uvec2 Out[];
};

layout(rgba8) readonly uniform image2D src0;
layout(rgba8) readonly uniform image2D src1;
uniform sampler2D uTex2D0;

uniform uvec4 uVec;
uniform uint max;
uniform mat3 uMat[16];
uniform mat3 uMat0;
uniform mat3 uMat1;
uniform mat3 uMat2;
uniform mat3 uMat3;

void main(void) {
	Out[uVec.x] = In[uVec.y];
}

