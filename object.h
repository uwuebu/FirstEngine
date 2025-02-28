#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

#include <SHADER/shader_c.h>

#include "imgui.h"

// Default object transforms values
const glm::vec3 POSITION(0.0f);
const glm::vec3 ROTATION(0.0f);
const glm::vec3 SIZE(1.0f);

// struct for object's tranforms

struct Transforms {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 size;
};

// OBJECT CLASS
//=-----------------------------=
class Object {
protected:
    unsigned int ID;
    bool visible = 1;
    bool selected = 0;
    Transforms transforms;
    
public:
    std::string name = "Object";
    // Constructors
    Object(Transforms t) : transforms(t) {}
    Object(glm::vec3 position = POSITION, glm::vec3 rotation = ROTATION, glm::vec3 size = SIZE) {
        transforms.position = position;
        transforms.rotation = rotation;
        transforms.size = size;
    }

    // Virtual destructor to ensure proper cleanup in derived classes
    virtual ~Object() {}

    // Getters
    virtual unsigned int GetID() const { return ID; }
    virtual bool getSelection() const { return selected; }
    virtual bool getVisibility() const { return visible; }

    virtual glm::vec3 getPosition() const { return transforms.position; }
    virtual glm::vec3 getRotation() const { return transforms.rotation; }
    virtual glm::vec3 getSize() const { return transforms.size; }

    // Setters
    virtual void SetID(unsigned int id) { ID = id; }
    virtual void setSelection(bool sel) { selected = sel; }
    virtual void setVisibility(bool vis) { visible = vis; }

    virtual void setPosition(const glm::vec3& pos) { transforms.position = pos; }
    virtual void setRotation(const glm::vec3& rot) { transforms.rotation = rot; }
    virtual void setSize(const glm::vec3& s) { transforms.size = s; }


    // Pure virtual function for child classes
    virtual void update(Shader* shader, int index) = 0; // Ensures derived classes implement this

    virtual void draw_menu() {
        if (ImGui::Begin(("Properties - " + name).c_str())) {
            ImGui::Text("ID: %u", ID);
            ImGui::Checkbox("Visible", &visible);

            // Transform controls
            ImGui::DragFloat3("Position", glm::value_ptr(transforms.position), 0.1f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(transforms.rotation), 0.1f);
            ImGui::DragFloat3("Size", glm::value_ptr(transforms.size), 0.1f);
        }
        ImGui::End();
    }
};
