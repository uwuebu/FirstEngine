//#version 420 core
//layout (triangles) in;
//layout (triangle_strip, max_vertices=18) out;
//
//uniform mat4 shadowMatrices[6];
//uniform int lightIndex;        
//
//out vec4 FragPos;
//
//void main()
//{
//    for(int face = 0; face < 6; ++face)
//    {
//        gl_Layer = (lightIndex * 6) + face; 
//        for(int i = 0; i < 3; ++i) 
//        {
//            FragPos = gl_in[i].gl_Position;
//            gl_Position = shadowMatrices[face] * FragPos;
//            EmitVertex();
//        }    
//        EndPrimitive();
//    }
//} 

#version 460 core

layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 3) out;

out vec4 FragPos;
uniform int lightIndex;

layout (std140, binding = 2) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices [6];
};

void main(){
	for (int i = 0; i < 3; ++i){
		FragPos = gl_in[i].gl_Position;
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = (lightIndex * 6) + gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
} 