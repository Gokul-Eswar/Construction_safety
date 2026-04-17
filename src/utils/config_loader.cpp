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
            config.detection.inference_interval = interval;
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
        // Detection Tuning Configuration Validation
        // ================================================================================
        if (j.contains("detection_tuning")) {
            auto& det = j["detection_tuning"];
            
            if (det.contains("confidence_threshold")) {
                float conf = det["confidence_threshold"];
                if (conf < 0.05f || conf > 0.95f) {
                    throw std::runtime_error("detection_tuning.confidence_threshold must be 0.05-0.95; got " + std::to_string(conf));
                }
                config.detection.confidence_threshold = conf;
            }
            
            if (det.contains("nms_threshold")) {
                float nms = det["nms_threshold"];
                if (nms < 0.1f || nms > 0.95f) {
                    throw std::runtime_error("detection_tuning.nms_threshold must be 0.1-0.95; got " + std::to_string(nms));
                }
                config.detection.nms_threshold = nms;
            }
            
            if (det.contains("inference_interval")) {
                int interval = det["inference_interval"];
                if (interval < 1 || interval > 30) {
                    throw std::runtime_error("detection_tuning.inference_interval must be 1-30; got " + std::to_string(interval));
                }
                config.detection.inference_interval = interval;
            }
            
            spdlog::info("Loaded detection_tuning: conf_threshold={}, nms_threshold={}, inference_interval={}",
                        config.detection.confidence_threshold, config.detection.nms_threshold, 
                        config.detection.inference_interval);
        }
        
        // ================================================================================
        // Tracking Tuning Configuration Validation (Occlusion Robustness)
        // ================================================================================
        if (j.contains("tracking_tuning")) {
            auto& track = j["tracking_tuning"];
            
            if (track.contains("max_age")) {
                int max_age = track["max_age"];
                if (max_age < 15 || max_age > 120) {
                    throw std::runtime_error("tracking_tuning.max_age must be 15-120; got " + std::to_string(max_age));
                }
                config.tracking.max_age = max_age;
            }
            
            if (track.contains("min_hits")) {
                int min_hits = track["min_hits"];
                if (min_hits < 1 || min_hits > 10) {
                    throw std::runtime_error("tracking_tuning.min_hits must be 1-10; got " + std::to_string(min_hits));
                }
                config.tracking.min_hits = min_hits;
            }
            
            if (track.contains("iou_threshold")) {
                float iou = track["iou_threshold"];
                if (iou < 0.1f || iou > 0.9f) {
                    throw std::runtime_error("tracking_tuning.iou_threshold must be 0.1-0.9; got " + std::to_string(iou));
                }
                config.tracking.iou_threshold = iou;
            }
            
            if (track.contains("feature_threshold")) {
                float feat = track["feature_threshold"];
                if (feat < 0.3f || feat > 0.9f) {
                    throw std::runtime_error("tracking_tuning.feature_threshold must be 0.3-0.9; got " + std::to_string(feat));
                }
                config.tracking.feature_threshold = feat;
            }
            
            if (track.contains("occlusion_extension_frames")) {
                int occ_ext = track["occlusion_extension_frames"];
                if (occ_ext < 5 || occ_ext > 60) {
                    throw std::runtime_error("tracking_tuning.occlusion_extension_frames must be 5-60; got " + std::to_string(occ_ext));
                }
                config.tracking.occlusion_extension_frames = occ_ext;
            }
            
            spdlog::info("Loaded tracking_tuning: max_age={}, min_hits={}, iou_threshold={}, feature_threshold={}, occlusion_extension_frames={}",
                        config.tracking.max_age, config.tracking.min_hits, config.tracking.iou_threshold,
                        config.tracking.feature_threshold, config.tracking.occlusion_extension_frames);
        }
        
        // ================================================================================
        // Preprocessing Configuration Validation (CLAHE for Extreme Lighting)
        // ================================================================================
        if (j.contains("preprocessing")) {
            auto& prep = j["preprocessing"];
            
            if (prep.contains("clahe_enabled")) {
                config.preprocessing.clahe_enabled = prep["clahe_enabled"];
            }
            
            if (prep.contains("clahe_clip_limit")) {
                float clip = prep["clahe_clip_limit"];
                if (clip < 1.0f || clip > 4.0f) {
                    throw std::runtime_error("preprocessing.clahe_clip_limit must be 1.0-4.0; got " + std::to_string(clip));
                }
                config.preprocessing.clahe_clip_limit = clip;
            }
            
            if (prep.contains("clahe_tile_size")) {
                int tile = prep["clahe_tile_size"];
                if (tile < 4 || tile > 16) {
                    throw std::runtime_error("preprocessing.clahe_tile_size must be 4-16; got " + std::to_string(tile));
                }
                config.preprocessing.clahe_tile_size = tile;
            }
            
            if (prep.contains("clahe_blur_kernel")) {
                int blur = prep["clahe_blur_kernel"];
                if (blur < 1 || blur > 7 || blur % 2 == 0) {
                    throw std::runtime_error("preprocessing.clahe_blur_kernel must be odd 1-7; got " + std::to_string(blur));
                }
                config.preprocessing.clahe_blur_kernel = blur;
            }
            
            spdlog::info("Loaded preprocessing: clahe_enabled={}, clahe_clip_limit={}, clahe_tile_size={}, clahe_blur_kernel={}",
                        config.preprocessing.clahe_enabled, config.preprocessing.clahe_clip_limit,
                        config.preprocessing.clahe_tile_size, config.preprocessing.clahe_blur_kernel);
        }
        
        // ================================================================================
        // Zone Detection Configuration Validation (Perspective Error Handling)
        // ================================================================================
        if (j.contains("zone_detection")) {
            auto& zone_det = j["zone_detection"];
            
            if (zone_det.contains("mode")) {
                std::string mode_str = zone_det["mode"];
                if (mode_str == "point") {
                    config.zone_detection.mode = ZoneDetectionConfig::Mode::POINT;
                } else if (mode_str == "footprint") {
                    config.zone_detection.mode = ZoneDetectionConfig::Mode::FOOTPRINT;
                } else if (mode_str == "calibrated") {
                    config.zone_detection.mode = ZoneDetectionConfig::Mode::CALIBRATED;
                } else {
                    throw std::runtime_error("zone_detection.mode must be 'point', 'footprint', or 'calibrated'; got " + mode_str);
                }
            }
            
            if (zone_det.contains("boundary_margin")) {
                float margin = zone_det["boundary_margin"];
                if (margin < -20.0f || margin > 20.0f) {
                    throw std::runtime_error("zone_detection.boundary_margin must be -20 to 20; got " + std::to_string(margin));
                }
                config.zone_detection.boundary_margin = margin;
            }
            
            if (zone_det.contains("footprint_voting")) {
                std::string voting_str = zone_det["footprint_voting"];
                if (voting_str == "all") {
                    config.zone_detection.footprint_voting = ZoneDetectionConfig::VotingStrategy::ALL;
                } else if (voting_str == "majority") {
                    config.zone_detection.footprint_voting = ZoneDetectionConfig::VotingStrategy::MAJORITY;
                } else if (voting_str == "any") {
                    config.zone_detection.footprint_voting = ZoneDetectionConfig::VotingStrategy::ANY;
                } else {
                    throw std::runtime_error("zone_detection.footprint_voting must be 'all', 'majority', or 'any'; got " + voting_str);
                }
            }
            
            std::string mode_name = config.zone_detection.mode == ZoneDetectionConfig::Mode::POINT ? "point" :
                                    config.zone_detection.mode == ZoneDetectionConfig::Mode::FOOTPRINT ? "footprint" : "calibrated";
            std::string voting_name = config.zone_detection.footprint_voting == ZoneDetectionConfig::VotingStrategy::ALL ? "all" :
                                      config.zone_detection.footprint_voting == ZoneDetectionConfig::VotingStrategy::MAJORITY ? "majority" : "any";
            spdlog::info("Loaded zone_detection: mode={}, boundary_margin={}, footprint_voting={}", 
                        mode_name, config.zone_detection.boundary_margin, voting_name);
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
            spdlog::warn("No streams configured. Engine will start in idle mode and remain online for camera onboarding.");
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
    j["stream_port"] = config.stream_port;
    
    // Detection tuning
    j["detection_tuning"]["confidence_threshold"] = config.detection.confidence_threshold;
    j["detection_tuning"]["nms_threshold"] = config.detection.nms_threshold;
    j["detection_tuning"]["inference_interval"] = config.detection.inference_interval;
    
    // Tracking tuning
    j["tracking_tuning"]["max_age"] = config.tracking.max_age;
    j["tracking_tuning"]["min_hits"] = config.tracking.min_hits;
    j["tracking_tuning"]["iou_threshold"] = config.tracking.iou_threshold;
    j["tracking_tuning"]["feature_threshold"] = config.tracking.feature_threshold;
    j["tracking_tuning"]["occlusion_extension_frames"] = config.tracking.occlusion_extension_frames;
    
    // Preprocessing config
    j["preprocessing"]["clahe_enabled"] = config.preprocessing.clahe_enabled;
    j["preprocessing"]["clahe_clip_limit"] = config.preprocessing.clahe_clip_limit;
    j["preprocessing"]["clahe_tile_size"] = config.preprocessing.clahe_tile_size;
    j["preprocessing"]["clahe_blur_kernel"] = config.preprocessing.clahe_blur_kernel;
    
    // Zone detection config
    j["zone_detection"]["mode"] = config.zone_detection.mode == ZoneDetectionConfig::Mode::POINT ? "point" :
                                   config.zone_detection.mode == ZoneDetectionConfig::Mode::FOOTPRINT ? "footprint" : "calibrated";
    j["zone_detection"]["boundary_margin"] = config.zone_detection.boundary_margin;
    j["zone_detection"]["footprint_voting"] = config.zone_detection.footprint_voting == ZoneDetectionConfig::VotingStrategy::ALL ? "all" :
                                              config.zone_detection.footprint_voting == ZoneDetectionConfig::VotingStrategy::MAJORITY ? "majority" : "any";
    
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
