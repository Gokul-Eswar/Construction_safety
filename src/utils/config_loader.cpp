#include "config_loader.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>

using json = nlohmann::json;

namespace {
int parseZoneId(const json& value) {
    if (value.is_number_integer()) {
        return value.get<int>();
    }

    if (value.is_string()) {
        const std::string id_str = value.get<std::string>();
        size_t parsed_chars = 0;
        const int parsed = std::stoi(id_str, &parsed_chars);
        if (parsed_chars == id_str.size()) {
            return parsed;
        }
    }

    throw std::runtime_error("Zone 'id' must be an integer or numeric string");
}
} // namespace

AppConfig ConfigLoader::load(const std::string& path) {
    AppConfig config;
    std::ifstream f(path);
    if (!f.good()) {
        throw std::runtime_error("Config file not found: " + path);
    }

    try {
        json j;
        f >> j;

        // Validate required fields
        if (!j.contains("streams") || !j["streams"].is_array()) {
            throw std::runtime_error("Config must contain 'streams' array");
        }

        if (j.contains("model_path")) config.model_path = j["model_path"];
        if (j.contains("database_path")) config.database_path = j["database_path"];
        if (j.contains("alert_cooldown")) config.alert_cooldown = j["alert_cooldown"];
        if (j.contains("log_retention_days")) config.log_retention_days = j["log_retention_days"];
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
                
                // Validate required stream fields
                if (!s.contains("id") || !s.contains("uri")) {
                    throw std::runtime_error("Each stream must have 'id' and 'uri' fields");
                }
                
                if (s.contains("id")) stream.id = s["id"];
                if (s.contains("name")) stream.name = s["name"];
                
                // Unified URI handling
                if (s.contains("uri")) {
                    stream.uri = s["uri"];
                } else if (s.contains("rtsp_uri")) {
                    stream.uri = s["rtsp_uri"];
                }

                if (s.contains("type")) {
                    stream.type = s["type"];
                } else {
                    // Inference type from uri
                    if (stream.uri == "test") stream.type = "test";
                    else if (stream.uri.find("rtsp://") == 0) stream.type = "rtsp";
                    else if (stream.uri.find("/dev/video") == 0 || (stream.uri.length() > 0 && std::isdigit(stream.uri[0]))) stream.type = "usb";
                }
                
                if (s.contains("zones")) {
                    for (const auto& z : s["zones"]) {
                        ZoneConfig zone;
                        
                        // Validate zone fields
                        if (!z.contains("id") || !z.contains("points") || !z["points"].is_array()) {
                            throw std::runtime_error("Each zone must have 'id' and 'points' array");
                        }
                        
                        if (z.contains("id")) zone.id = parseZoneId(z["id"]);
                        if (z.contains("name")) zone.name = z["name"];
                        if (z.contains("points")) {
                            for (const auto& p : z["points"]) {
                                if (p.size() == 2 && p[0].is_number() && p[1].is_number()) {
                                    zone.points.emplace_back(p[0], p[1]);
                                } else {
                                    throw std::runtime_error("Zone points must be [x,y] number pairs");
                                }
                            }
                        }
                        stream.zones.push_back(zone);
                    }
                }

                if (s.contains("calibration")) {
                    for (const auto& c : s["calibration"]) {
                        if (c.contains("image") && c.contains("world")) {
                            CalibrationPoint cp;
                            cp.image = cv::Point2f(c["image"][0], c["image"][1]);
                            cp.world = cv::Point2f(c["world"][0], c["world"][1]);
                            stream.calibration.push_back(cp);
                        }
                    }
                }

                config.streams.push_back(stream);
            }
        }
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Config validation error: " + std::string(e.what()));
    }

    return config;
}

bool ConfigLoader::save(const std::string& path, const AppConfig& config) {
    json j;
    j["model_path"] = config.model_path;
    j["database_path"] = config.database_path;
    j["alert_cooldown"] = config.alert_cooldown;
    j["log_retention_days"] = config.log_retention_days;
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
        stream_json["type"] = s.type;
        stream_json["uri"] = s.uri;
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

        stream_json["calibration"] = json::array();
        for (const auto& c : s.calibration) {
            json cal_json;
            cal_json["image"] = {c.image.x, c.image.y};
            cal_json["world"] = {c.world.x, c.world.y};
            stream_json["calibration"].push_back(cal_json);
        }

        j["streams"].push_back(stream_json);
    }

    std::ofstream f(path);
    if (!f.good()) return false;
    f << j.dump(4);
    return true;
}