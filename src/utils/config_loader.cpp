#include "config_loader.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include <nlohmann/json.hpp>
#include <cctype>
#include <cstdlib>
#include <string>
#include <spdlog/spdlog.h>

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

        // ================================================================================
        // SCHEMA VALIDATION - Enforces config.schema.json constraints
        // ================================================================================
        
        // Required fields check
        if (!j.contains("streams") || !j["streams"].is_array()) {
            throw std::runtime_error("Config must contain 'streams' array (required by schema)");
        }
        
        if (!j.contains("mqtt") || !j["mqtt"].is_object()) {
            throw std::runtime_error("Config must contain 'mqtt' object (required by schema)");
        }
        
        // Global field parsing with range validation
        if (j.contains("model_path")) config.model_path = j["model_path"];
        if (j.contains("database_path")) config.database_path = j["database_path"];
        
        // Alert cooldown: 1000-300000ms (1-300 seconds)
        if (j.contains("alert_cooldown")) {
            int cooldown = j["alert_cooldown"];
            if (cooldown < 1000 || cooldown > 300000) {
                throw std::runtime_error("alert_cooldown must be between 1000-300000ms; got " + std::to_string(cooldown));
            }
            config.alert_cooldown = cooldown;
        }
        
        // Log retention: 1-365 days
        if (j.contains("log_retention_days")) {
            int days = j["log_retention_days"];
            if (days < 1 || days > 365) {
                throw std::runtime_error("log_retention_days must be between 1-365; got " + std::to_string(days));
            }
            config.log_retention_days = days;
        }
        
        // Inference interval: 1-30
        if (j.contains("inference_interval")) {
            int interval = j["inference_interval"];
            if (interval < 1 || interval > 30) {
                throw std::runtime_error("inference_interval must be between 1-30; got " + std::to_string(interval));
            }
            config.inference_interval = interval;
        }
        
        // Stream port: 1024-65535
        if (j.contains("stream_port")) {
            int port = j["stream_port"];
            if (port < 1024 || port > 65535) {
                throw std::runtime_error("stream_port must be between 1024-65535; got " + std::to_string(port));
            }
            config.stream_port = port;
        }
        
        // ================================================================================
        // MQTT Configuration Validation
        // ================================================================================
        auto& mqtt_obj = j["mqtt"];
        
        // Required MQTT fields
        if (!mqtt_obj.contains("host") || !mqtt_obj["host"].is_string()) {
            throw std::runtime_error("mqtt.host is required (string)");
        }
        if (!mqtt_obj.contains("port") || !mqtt_obj["port"].is_number()) {
            throw std::runtime_error("mqtt.port is required (integer)");
        }
        
        int mqtt_port = mqtt_obj["port"];
        if (mqtt_port < 1 || mqtt_port > 65535) {
            throw std::runtime_error("mqtt.port must be between 1-65535; got " + std::to_string(mqtt_port));
        }
        
        config.mqtt.host = mqtt_obj["host"];
        config.mqtt.port = mqtt_port;
        
        // Topic should use v1 format for versioning (but accept both for compatibility)
        if (mqtt_obj.contains("topic")) {
            std::string topic = mqtt_obj["topic"];
            if (topic.empty()) {
                spdlog::warn("mqtt.topic is empty, using default 'safety/v1/violations'");
                config.mqtt.topic = "safety/v1/violations";
            } else {
                if (topic.find("/v") == std::string::npos) {
                    spdlog::warn("mqtt.topic should use versioning (e.g., 'safety/v1/violations'), got: {}. "
                                 "See docs/MQTT_EVENT_CONTRACT.md", topic);
                }
                config.mqtt.topic = topic;
            }
        } else {
            config.mqtt.topic = "safety/v1/violations";
        }
        
        if (mqtt_obj.contains("client_id")) {
            config.mqtt.client_id = mqtt_obj["client_id"];
        }
        
        // Optional MQTT credentials
        if (mqtt_obj.contains("username")) config.mqtt.client_id = mqtt_obj["username"];
        if (mqtt_obj.contains("password")) {} // Password handled separately (not stored in config)
        
        // Environment Override
        const char* env_mqtt_host = std::getenv("MQTT_HOST");
        if (env_mqtt_host) {
            config.mqtt.host = env_mqtt_host;
            spdlog::info("Overriding MQTT Host from ENV: {}", env_mqtt_host);
        }

        // ================================================================================
        // Stream Configuration Validation
        // ================================================================================
        if (j["streams"].size() == 0) {
            throw std::runtime_error("streams array must contain at least 1 stream (schema requires minItems: 1)");
        }
        
        for (size_t stream_idx = 0; stream_idx < j["streams"].size(); ++stream_idx) {
            const auto& s = j["streams"][stream_idx];
            StreamConfig stream;
            
            // Validate required stream fields
            if (!s.contains("id") || !s.contains("uri")) {
                throw std::runtime_error("Stream[" + std::to_string(stream_idx) + 
                                         "]: must have 'id' and 'uri' fields (required by schema)");
            }
            
            // Stream ID validation: alphanumeric, underscore, hyphen
            std::string stream_id = s["id"];
            if (stream_id.empty()) {
                throw std::runtime_error("Stream[" + std::to_string(stream_idx) + 
                                         "]: 'id' cannot be empty");
            }
            const std::regex id_pattern("^[a-zA-Z0-9_-]+$");
            if (!std::regex_match(stream_id, id_pattern)) {
                throw std::runtime_error("Stream[" + std::to_string(stream_idx) + 
                                         "]: 'id' must be alphanumeric with underscore/hyphen; got '" + stream_id + "'");
            }
            stream.id = stream_id;
            
            if (s.contains("name")) stream.name = s["name"];
            
            // Unified URI handling
            if (s.contains("uri")) {
                stream.uri = s["uri"];
            } else if (s.contains("rtsp_uri")) {
                stream.uri = s["rtsp_uri"];
            }
            
            if (stream.uri.empty()) {
                throw std::runtime_error("Stream[" + stream.id + "]: 'uri' cannot be empty");
            }

            if (s.contains("type")) {
                stream.type = s["type"];
            } else {
                if (stream.uri.find("/dev/video") == 0 || (stream.uri.length() > 0 && std::isdigit(static_cast<unsigned char>(stream.uri[0])))) {
                    stream.type = "usb";
                } else {
                    stream.type = "rtsp";
                }
            }

            if (stream.type != "rtsp" && stream.type != "usb" && stream.type != "http") {
                throw std::runtime_error("Stream[" + stream.id + 
                                         "]: Unsupported stream type: " + stream.type + 
                                         " (schema allows: rtsp, usb, http)");
            }
            
            // ================================================================================
            // Zone Configuration Validation  
            // ================================================================================
            if (s.contains("zones")) {
                if (!s["zones"].is_array()) {
                    throw std::runtime_error("Stream[" + stream.id + "]: 'zones' must be an array");
                }
                
                if (s["zones"].size() > 0 && s["zones"].size() < 1) {
                    throw std::runtime_error("Stream[" + stream.id + 
                                             "]: 'zones' array must have at least 1 zone if present");
                }
                
                for (size_t zone_idx = 0; zone_idx < s["zones"].size(); ++zone_idx) {
                    const auto& z = s["zones"][zone_idx];
                    ZoneConfig zone;
                    
                    // Validate required zone fields
                    if (!z.contains("id") || !z.contains("points") || !z["points"].is_array()) {
                        throw std::runtime_error("Stream[" + stream.id + "].Zone[" + std::to_string(zone_idx) + 
                                                 "]: must have 'id' and 'points' array (required by schema)");
                    }
                    
                    // Zone ID: integer or numeric string
                    if (z.contains("id")) zone.id = parseZoneId(z["id"]);
                    if (z.contains("name")) zone.name = z["name"];
                    
                    // Zone points: minimum 3 points (triangle), each [x,y]
                    if (z["points"].size() < 3) {
                        throw std::runtime_error("Stream[" + stream.id + "].Zone[" + std::to_string(zone_idx) + 
                                                 "]: 'points' must have at least 3 points; got " + 
                                                 std::to_string(z["points"].size()));
                    }
                    
                    for (size_t pt_idx = 0; pt_idx < z["points"].size(); ++pt_idx) {
                        const auto& p = z["points"][pt_idx];
                        if (p.size() != 2 || !p[0].is_number() || !p[1].is_number()) {
                            throw std::runtime_error("Stream[" + stream.id + "].Zone[" + std::to_string(zone_idx) + 
                                                     "].Point[" + std::to_string(pt_idx) + 
                                                     "]: must be [x,y] number pair; got " + p.dump());
                        }
                        zone.points.emplace_back(p[0], p[1]);
                    }
                    stream.zones.push_back(zone);
                }
            }

            // ================================================================================
            // Calibration Points (Optional)
            // ================================================================================
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
        
        spdlog::info("Config loaded and validated successfully (via config.schema.json constraints)");
        
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()) + 
                                 "\nEnsure JSON is valid. See config.json.example for correct format.");
    } catch (const std::exception& e) {
        throw std::runtime_error("Config validation error: " + std::string(e.what()) + 
                                 "\nCheck against schema: config.schema.json");
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
