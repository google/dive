#version 310 es
#extension GL_OES_shader_image_atomic : enable

precision highp float;
precision highp int;

layout(local_size_x=5, local_size_y=6, local_size_z=7) in;

layout(rgba8) readonly uniform image2D uImage2D0;
layout(rgba8) writeonly uniform image2D uImage2D1;

void main(void) {
	imageStore(uImage2D1, ivec2(3, 4), imageLoad(uImage2D0, ivec2(3, 4)));
}

