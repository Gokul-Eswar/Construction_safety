#include "config_loader.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

AppConfig ConfigLoader::load(const std::string& path) {
    AppConfig config;
    std::ifstream f(path);
    if (!f.good()) {
        std::cerr << "Config file not found: " << path << ". Using defaults." << std::endl;
        return config;
    }

    try {
        json j;
        f >> j;

        if (j.contains("rtsp_uri")) config.rtsp_uri = j["rtsp_uri"];
        if (j.contains("model_path")) config.model_path = j["model_path"];
        if (j.contains("database_path")) config.database_path = j["database_path"];
        if (j.contains("alert_cooldown")) config.alert_cooldown = j["alert_cooldown"];
        
        if (j.contains("mqtt")) {
            auto& m = j["mqtt"];
            if (m.contains("host")) config.mqtt.host = m["host"];
            if (m.contains("port")) config.mqtt.port = m["port"];
            if (m.contains("topic")) config.mqtt.topic = m["topic"];
            if (m.contains("client_id")) config.mqtt.client_id = m["client_id"];
        }

        if (j.contains("zones")) {
            for (const auto& z : j["zones"]) {
                ZoneConfig zone;
                if (z.contains("id")) zone.id = z["id"];
                if (z.contains("name")) zone.name = z["name"];
                if (z.contains("points")) {
                    for (const auto& p : z["points"]) {
                        if (p.size() == 2) {
                            zone.points.emplace_back(p[0], p[1]);
                        }
                    }
                }
                config.zones.push_back(zone);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
    }

    return config;
}

bool ConfigLoader::save(const std::string& path, const AppConfig& config) {
    json j;
    j["rtsp_uri"] = config.rtsp_uri;
    j["model_path"] = config.model_path;
    j["database_path"] = config.database_path;
    j["alert_cooldown"] = config.alert_cooldown;
    
    j["mqtt"]["host"] = config.mqtt.host;
    j["mqtt"]["port"] = config.mqtt.port;
    j["mqtt"]["topic"] = config.mqtt.topic;
    j["mqtt"]["client_id"] = config.mqtt.client_id;
    
    j["zones"] = json::array();
    for (const auto& z : config.zones) {
        json zone_json;
        zone_json["id"] = z.id;
        zone_json["name"] = z.name;
        zone_json["points"] = json::array();
        for (const auto& p : z.points) {
            zone_json["points"].push_back({p.x, p.y});
        }
        j["zones"].push_back(zone_json);
    }

    std::ofstream f(path);
    if (!f.good()) return false;
    f << j.dump(4);
    return true;
}
