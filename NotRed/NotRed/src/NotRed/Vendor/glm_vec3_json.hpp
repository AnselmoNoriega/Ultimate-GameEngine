#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>

#include <glm/vec3.hpp>
#include <json.hpp>

namespace NR { struct Vertex; }

namespace nlohmann::json_abi_v3_11_3::detail
{
    static void to_json(json& j, const glm::vec3& vec) {
        j = json{ {"x", vec.x}, {"y", vec.y}, {"z", vec.z} };
    }

    static void from_json(const json& j, glm::vec3& vec) {
        j.at("x").get_to(vec.x);
        j.at("y").get_to(vec.y);
        j.at("z").get_to(vec.z);
    }

    static void to_json(json& j, const std::vector<glm::vec3>& vec) {
        j = json::array();
        for (const auto& v : vec) {
            j.push_back(v);
        }
    }

    static void from_json(const json& j, std::vector<glm::vec3>& vec) {
        vec.clear();
        for (const auto& item : j) {
            vec.push_back(item.get<glm::vec3>());
        }
    }

    static void to_json(json& j, const glm::vec2& vec) {
        j = json{ {"x", vec.x}, {"y", vec.y} };
    }

    static void from_json(const json& j, glm::vec2& vec) {
        j.at("x").get_to(vec.x);
        j.at("y").get_to(vec.y);
    }

    static void to_json(json& j, const std::vector<glm::vec2>& vec) {
        j = json::array();
        for (const auto& v : vec) {
            j.push_back(v);
        }
    }

    static void from_json(const json& j, std::vector<glm::vec2>& vec) {
        vec.clear();
        for (const auto& item : j) {
            vec.push_back(item.get<glm::vec2>());
        }
    }

    void to_json(json& j, const NR::Vertex& vertex);
    void from_json(const json& j, NR::Vertex& vertex);

    void to_json(json& j, const std::vector<NR::Vertex>& vertices);
    void from_json(const json& j, std::vector<NR::Vertex>& vertices);
}