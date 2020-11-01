#version 130

//out vec4 fColor;
in vec3 color;
void main()
{
    gl_FragColor = vec4(color, 0.0);
}
