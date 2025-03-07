#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;
layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};
void main()
{
	TexCoords = aPos;
	mat4 view_c = mat4(mat3(view));
	vec4 pos = projection * view_c * vec4(aPos, 1.0);
	gl_Position = pos.xyww;
}
