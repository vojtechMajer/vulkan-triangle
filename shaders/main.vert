#version 450
// Vertex shader 
// is invoked for every vertex gl_vertexIndex stores index of that vertex

// gl_Position - contains position of current vertex
// some types like dvec3 takes multiple slots meaning that  
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {

    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;

}
