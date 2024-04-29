#version 450

layout(location = 0) in vec4 vert;

void main() {

    gl_Position = vert;
}