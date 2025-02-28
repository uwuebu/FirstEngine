#pragma once
#include "object.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include <SHADER/shader_c.h>
#include "stb_image.h"

struct Material {
	glm::vec3 diffuse = glm::vec3(1.0f);;
	glm::vec3 specular = glm::vec3(1.0f);;
	float shininess = 128.0f;
};

class Model : public Object{
public:
	Material material;
	bool scale_texture = false;
	Model(char* path){
		loadModel(path);
	}
	Model(Mesh mesh) {
		this->meshes.push_back(mesh);
	}
	void Draw(Shader* shader);
	void DrawDepth(Shader* shader);
	void DrawStencil(Shader* shader);
	Material getMaterial() { return material; }
	void setDiffuse(glm::vec3 diffuse) { material.diffuse = diffuse; }
	void setSpecular(glm::vec3 specular) { material.specular = specular; }
	void setShininess(float shininess) { material.shininess = shininess; }
private:
	// model data
	vector<Mesh> meshes;
	string directory;
	vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
	void loadModel(string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
	void update(Shader* shader, int index) override {};
	void draw_menu() override {
		if (ImGui::Begin(("Properties - " + name).c_str())) {
			Object::draw_menu();
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(material.diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(material.specular));
			ImGui::DragFloat("Shininess", &material.shininess, 1.0f, 500.0f, 10.0f);
		}
		ImGui::End();
	}
};

void Model::Draw(Shader* shader) {
	if (visible) {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, transforms.position);
		model = glm::rotate(model, glm::radians(transforms.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
		model = glm::rotate(model, glm::radians(transforms.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y-axis
		model = glm::rotate(model, glm::radians(transforms.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z-axis
		model = glm::scale(model, glm::vec3(transforms.size));

		shader->use();
		shader->setMat4("model", model);
		shader->setBool("useTextureScaling", scale_texture);
		shader->setVec3("material.diffuse", material.diffuse);
		shader->setVec3("material.specular", material.specular);
		shader->setFloat("material.shininess", material.shininess);

		// Draw the object
		for (unsigned int i = 0; i < meshes.size(); i++) {
			if (scale_texture) {
				shader->setVec3("objectScale", transforms.size);
			}
			meshes[i].Draw(*shader);
		}
	}
}

void Model::DrawDepth(Shader* shader) {
	if (visible) {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, transforms.position);
		model = glm::rotate(model, glm::radians(transforms.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
		model = glm::rotate(model, glm::radians(transforms.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y-axis
		model = glm::rotate(model, glm::radians(transforms.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z-axis
		model = glm::scale(model, glm::vec3(transforms.size));

		shader->use();
		shader->setMat4("model", model);
		// Draw the object
		for (unsigned int i = 0; i < meshes.size(); i++) {
			meshes[i].Draw(*shader);
		}
	}
}

void Model::DrawStencil(Shader* select_shader){
	// Draw the outline
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, transforms.position);
	model = glm::rotate(model, glm::radians(transforms.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
	model = glm::rotate(model, glm::radians(transforms.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y-axis
	model = glm::rotate(model, glm::radians(transforms.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z-axis
	model = glm::scale(model, glm::vec3(transforms.size));
	//model = model + glm::mat4(0.03);
	model = glm::scale(model, glm::vec3(1.04f)); // Scale up for outline

	select_shader->use();
	select_shader->setMat4("model", model);
	for (unsigned int i = 0; i < meshes.size(); i++) {
		meshes[i].Draw(*select_shader);
	}
}

void Model:: loadModel(string path){
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate |
		aiProcess_FlipUVs);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene){
	// process all the node’s meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++){
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++){
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene){
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;
	for (unsigned int i = 0; i < mesh->mNumVertices; i++){
		Vertex vertex;
		// process vertex positions
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;

		//process vertex normals
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;

		//process vertex texture coord
		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}
	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++){
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	// process material
	if (mesh->mMaterialIndex >= 0){
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		vector<Texture> diffuseMaps = loadMaterialTextures(material,aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(),
		specularMaps.end());
	}

	return Mesh(vertices, indices, textures);
}

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
	vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		for (unsigned int j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(textures_loaded[j]);
				skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}
		if (!skip)
		{   // if texture hasn't been loaded already, load it
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), this->directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
		}
	}
	return textures;
}

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format1, format2;
		if (nrComponents == 1) {
			format1 = GL_RED;
			format2 = format1;
		}
		else if (nrComponents == 3) {
			format1 = GL_SRGB;
			format2 = GL_RGB;
		}
		else if (nrComponents == 4) {
			format1 = GL_SRGB_ALPHA;
			format2 = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format1, width, height, 0, format2, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}
