attribute vec4 aPosition;
varying float foo;

void main()
{
	foo = log(aPosition.z);
	gl_Position = aPosition;
}
