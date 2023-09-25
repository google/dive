#version 310 es
// compare to 0017.cs

precision highp float;
precision highp int;

layout(local_size_x=5, local_size_y=6, local_size_z=7) in;

layout(binding = 0) buffer buffer_Out {
    int Out[];
};

void main(void) {
	Out[2] = atomicCompSwap(Out[3], 3, 5);
}

