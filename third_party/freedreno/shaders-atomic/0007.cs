#version 310 es
#extension GL_OES_shader_image_atomic : enable
// compare to 0001.cs

precision highp float;
precision highp int;

layout(local_size_x=5, local_size_y=6, local_size_z=7) in;

layout(binding = 0) buffer buffer_Out {
    int Out[];
};

layout(r32i) uniform iimage2D tex;

void main(void) {
	Out[2] = imageAtomicAdd(tex, ivec2(Out[5], Out[5]), 7);
}

