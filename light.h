#pragma once

#include "object.h"
#include <custom/camera.h>

struct LightColor {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

// Light class (general)
class Light : public Object {
protected:
    LightColor color;
    float intensity;
    
public:
    // Constructors
    Light(glm::vec3 position = POSITION,
        LightColor color = { glm::vec3(0.2f), glm::vec3(1.0f), glm::vec3(1.0f) },
        float intensity = 1.0f)
        : Object(position), color(color), intensity(intensity) {
    }

    // Getters
    LightColor getColor() const { return color; }
    float getIntensity() const { return intensity; }

    // Setters
    void setColor(const LightColor& c) { color = c; }
    void setIntensity(float i) { intensity = i; }

    // Override update function
    virtual void update(Shader* shader, int index) = 0;

    void draw_menu() override {
        if (ImGui::Begin(("Properties - " + name).c_str())) {
            ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 10.0f);
        }
        ImGui::End();
    }
};

class AmbientLight : public Light {
public:
    AmbientLight(glm::vec3 am_color = glm::vec3(1.0f), float intens = 0.3f) {
        color.ambient = am_color;
        intensity = intens;
    }
    void update(Shader* shader, int index) {
        shader->use();
        shader->setVec3("ambient", glm::vec3(color.ambient * intensity));
    }
    void draw_menu() override {
        if (ImGui::Begin(("Properties - " + name).c_str())) {
            Light::draw_menu();  // Call base class UI
            ImGui::ColorEdit3("Light Color", glm::value_ptr(color.ambient));
        }
        ImGui::End();
    }
};

const unsigned int CASCADE_COUNT = 4;
// Directional Light
class DirectionalLight : public Light {
private:
    glm::vec3 direction;
   
    //unsigned int depthMapFBO;
    //unsigned int depthMap;

    Camera* camera;
public:
    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    unsigned int depthMapFBO;  // Framebuffers for cascades
    unsigned int depthMap;    // Depth maps
    std::vector<float> shadowCascadeLevels;

    DirectionalLight(Camera* camera_p, glm::vec3 direction = glm::vec3(-1.0f, -1.0f, -1.0f),
        LightColor color = { glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f) },
        float intensity = 0.3f)
        : Light(glm::vec3(0.0f), color, intensity), direction(glm::normalize(direction)) {
        /*setupDepthBuffer();*/
        camera = camera_p;
        setupDepthBuffers();
    }

    glm::vec3 getDirection() const { return direction; }

    void setDirection(const glm::vec3& dir) { direction = glm::normalize(dir); }
    void setCamera(Camera* camera_s) { camera = camera_s; }

    void update(Shader* shader, int index) override {
        shader->use();
        shader->setVec3("dirLight.direction", direction);
        shader->setVec3("dirLight.diffuse", color.diffuse * intensity);
        shader->setVec3("dirLight.specular", color.specular * intensity);

        shader->setInt("shadowMap", 3);
    }

    void setupDepthBuffers() {
        shadowCascadeLevels = std::vector<float>{ camera->getFar() / 150.0f, camera->getFar() / 50.0f, camera->getFar() / 18.0f, camera->getFar() / 5.0f };
        glGenFramebuffers(1, &depthMapFBO);

        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);
        glTexImage3D(
            GL_TEXTURE_2D_ARRAY,
            0,
            GL_DEPTH_COMPONENT32F,
            SHADOW_WIDTH,
            SHADOW_HEIGHT,
            int(shadowCascadeLevels.size()) + 1,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
            throw 0;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane)
    {
        const auto proj = glm::perspective(
            glm::radians(camera->Zoom),
            (float)camera->screenWidth / (float)camera->screenHeight,
            nearPlane, farPlane
        );
        const auto corners = camera->getFrustumCornersWorldSpace(proj, camera->GetViewMatrix());

        glm::vec3 center(0.0f);
        for (const auto& v : corners)
        {
            center += glm::vec3(v);
        }
        center /= static_cast<float>(corners.size());

        const auto lightView = glm::lookAt(center - direction, center, glm::vec3(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();

        for (const auto& v : corners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        // Define fixed scene bounds (10x10x10)
        constexpr float SCENE_BOUND = 10.0f; // Half-size for ±5 units
        minX = std::min(minX, -SCENE_BOUND);
        maxX = std::max(maxX, SCENE_BOUND);
        minY = std::min(minY, -SCENE_BOUND);
        maxY = std::max(maxY, SCENE_BOUND);
        minZ = std::min(minZ, -SCENE_BOUND);
        maxZ = std::max(maxZ, SCENE_BOUND);

        const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        return lightProjection * lightView;
    }

    std::vector<glm::mat4> getLightSpaceMatrices()
    {
        std::vector<glm::mat4> ret;
        for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
        {
            if (i == 0)
            {
                ret.push_back(getLightSpaceMatrix(camera->getNear(), shadowCascadeLevels[i]));
            }
            else if (i < shadowCascadeLevels.size())
            {
                ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
            }
            else
            {
                ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], camera->getFar()));
            }
        }
        return ret;
    }


    void draw_menu() override {
        if (ImGui::Begin(("Properties - " + name).c_str())) {
            Light::draw_menu();  // Call base class UI
            ImGui::ColorEdit3("Light Color Diffuse", glm::value_ptr(color.diffuse));
            ImGui::ColorEdit3("Light Color Specular", glm::value_ptr(color.specular));
            ImGui::DragFloat3("Direction", glm::value_ptr(direction), 0.1f);
        }
        ImGui::End();
    }

};

// Point Light
class PointLight : public Light {
    Camera* camera;
public:
    //const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    float constant;
    float linear;
    float quadratic;
    unsigned int depthMapFBO;
    unsigned int depthMap;
    float near_plane = 0.1f;
    float far_plane = 100.0f;

    PointLight(Camera* camera_p, glm::vec3 position = POSITION,
        LightColor color = { glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f) },
        float intensity = 1.0f,
        float constant = 1.0f,
        float linear = 0.09f,
        float quadratic = 0.032f)
        : Light(position, color, intensity), constant(constant), linear(linear), quadratic(quadratic) {
        camera = camera_p;
    }
    void update(Shader* shader, int index) override {
        std::string prefix = "pointLights[" + std::to_string(index) + "]";

        shader->use();
        shader->setVec3(prefix + ".position", transforms.position);
        shader->setVec3(prefix + ".diffuse", color.diffuse * intensity);
        shader->setVec3(prefix + ".specular", color.specular * intensity);
        shader->setFloat(prefix + ".constant", constant);
        shader->setFloat(prefix + ".linear", linear);
        shader->setFloat(prefix + ".quadratic", quadratic);
    }

    std::vector<glm::mat4> getLightSpaceMatrix(unsigned int shadow_resolution) {
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)shadow_resolution/(float)shadow_resolution, near_plane, far_plane);

        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(transforms.position, transforms.position + glm::vec3(1.0, 0.0, 0.0) , glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(transforms.position, transforms.position + glm::vec3(-1.0, 0.0, 0.0) , glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(transforms.position, transforms.position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(transforms.position, transforms.position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(transforms.position, transforms.position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(transforms.position, transforms.position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

        return shadowTransforms;
    }

    void draw_menu() override {
        if (ImGui::Begin(("Properties - " + name).c_str())) {
            Object::draw_menu();
            Light::draw_menu();  // Call base class UI
            ImGui::ColorEdit3("Light Color Diffuse", glm::value_ptr(color.diffuse));
            ImGui::ColorEdit3("Light Color Specular", glm::value_ptr(color.specular));
            ImGui::DragFloat("Constant", &constant, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Linear", &linear, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Quadratic", &quadratic, 0.0001f, 0.0f, 0.1f);
        }
        ImGui::End();
    }
};

// Spot Light
class SpotLight : public Light {
private:
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;

public:
    // Constructor
    SpotLight(glm::vec3 position = POSITION,
        glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f),
        float cutOff = glm::cos(glm::radians(12.5f)),
        float outerCutOff = glm::cos(glm::radians(15.0f)),
        LightColor color = { glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(1.0f) },
        float intensity = 1.0f,
        float constant = 1.0f,
        float linear = 0.09f,
        float quadratic = 0.032f)
        : Light(position, color, intensity),
        direction(glm::normalize(direction)),
        cutOff(cutOff),
        outerCutOff(outerCutOff),
        constant(constant),
        linear(linear),
        quadratic(quadratic) {
    }

    // Getters
    glm::vec3 getDirection() const { return direction; }
    float getCutOff() const { return cutOff; }
    float getOuterCutOff() const { return outerCutOff; }

    // Setters
    void setDirection(const glm::vec3& dir) { direction = glm::normalize(dir); }
    void setCutOff(float cutOffValue) { cutOff = cutOffValue; }
    void setOuterCutOff(float outerCutOffValue) { outerCutOff = outerCutOffValue; }

    void update(Shader* shader, int index) override {
        std::string prefix = "spotLights[" + std::to_string(index) + "]";

        shader->use();
        shader->setVec3(prefix + ".position", transforms.position);
        shader->setVec3(prefix + ".direction", direction);
        shader->setVec3(prefix + ".diffuse", color.diffuse * intensity);
        shader->setVec3(prefix + ".specular", color.specular * intensity);
        shader->setFloat(prefix + ".cutOff", cutOff);
        shader->setFloat(prefix + ".outerCutOff", outerCutOff);
        shader->setFloat(prefix + ".constant", constant);
        shader->setFloat(prefix + ".linear", linear);
        shader->setFloat(prefix + ".quadratic", quadratic);
    }
};
