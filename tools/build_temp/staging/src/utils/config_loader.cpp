#include "config_loader.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include <cstdlib>

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

        if (j.contains("model_path")) config.model_path = j["model_path"];
        if (j.contains("database_path")) config.database_path = j["database_path"];
        if (j.contains("alert_cooldown")) config.alert_cooldown = j["alert_cooldown"];
        if (j.contains("inference_interval")) config.inference_interval = j["inference_interval"];
        if (j.contains("stream_port")) config.stream_port = j["stream_port"];
        
        if (j.contains("mqtt")) {
            auto& m = j["mqtt"];
            if (m.contains("host")) config.mqtt.host = m["host"];
            if (m.contains("port")) config.mqtt.port = m["port"];
            if (m.contains("topic")) config.mqtt.topic = m["topic"];
            if (m.contains("client_id")) config.mqtt.client_id = m["client_id"];
        }

        // Environment Override
        const char* env_mqtt_host = std::getenv("MQTT_HOST");
        if (env_mqtt_host) {
            config.mqtt.host = env_mqtt_host;
            std::cout << "Overriding MQTT Host from ENV: " << env_mqtt_host << std::endl;
        }

        if (j.contains("streams")) {
            for (const auto& s : j["streams"]) {
                StreamConfig stream;
                if (s.contains("id")) stream.id = s["id"];
                if (s.contains("name")) stream.name = s["name"];
                if (s.contains("rtsp_uri")) stream.rtsp_uri = s["rtsp_uri"];
                
                if (s.contains("zones")) {
                    for (const auto& z : s["zones"]) {
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
                        stream.zones.push_back(zone);
                    }
                }
                config.streams.push_back(stream);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
    }

    return config;
}

bool ConfigLoader::save(const std::string& path, const AppConfig& config) {
    json j;
    j["model_path"] = config.model_path;
    j["database_path"] = config.database_path;
    j["alert_cooldown"] = config.alert_cooldown;
    j["inference_interval"] = config.inference_interval;
    j["stream_port"] = config.stream_port;
    
    j["mqtt"]["host"] = config.mqtt.host;
    j["mqtt"]["port"] = config.mqtt.port;
    j["mqtt"]["topic"] = config.mqtt.topic;
    j["mqtt"]["client_id"] = config.mqtt.client_id;
    
    j["streams"] = json::array();
    for (const auto& s : config.streams) {
        json stream_json;
        stream_json["id"] = s.id;
        stream_json["name"] = s.name;
        stream_json["rtsp_uri"] = s.rtsp_uri;
        stream_json["zones"] = json::array();
        for (const auto& z : s.zones) {
            json zone_json;
            zone_json["id"] = z.id;
            zone_json["name"] = z.name;
            zone_json["points"] = json::array();
            for (const auto& p : z.points) {
                zone_json["points"].push_back({p.x, p.y});
            }
            stream_json["zones"].push_back(zone_json);
        }
        j["streams"].push_back(stream_json);
    }

    std::ofstream f(path);
    if (!f.good()) return false;
    f << j.dump(4);
    return true;
}