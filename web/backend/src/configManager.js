const fs = require('fs');
const path = require('path');

// Use CONFIG_PATH env var if available, otherwise resolve from this file's location.
const configPath = process.env.CONFIG_PATH || path.resolve(__dirname, '../../../config.json');
const exampleConfigPath = path.resolve(__dirname, '../../../config.json.example');
let projectConfig = {};

const loadConfig = () => {
    try {
        const resolvedConfigPath = fs.existsSync(configPath)
            ? configPath
            : (fs.existsSync(exampleConfigPath) ? exampleConfigPath : null);

        if (!resolvedConfigPath) {
            console.warn(`Config file not found at ${configPath} or ${exampleConfigPath}. Using empty config.`);
            return {};
        }
        const rawData = fs.readFileSync(resolvedConfigPath);
        projectConfig = JSON.parse(rawData);
        return projectConfig;
    } catch (error) {
        console.error(`Could not load config.json from ${configPath}:`, error);
        return {};
    }
};

const getConfig = () => {
    // Always reload to ensure freshness in this file-based architecture
    // In a real DB setup, we would cache this.
    return loadConfig();
};

const saveConfig = (newConfig) => {
    try {
        fs.writeFileSync(configPath, JSON.stringify(newConfig, null, 4));
        projectConfig = newConfig;
        return true;
    } catch (err) {
        console.error("Failed to save config:", err);
        return false;
    }
};

const updateGlobalSettings = (settings) => {
    loadConfig();

    if (settings && typeof settings === 'object') {
        if (settings.mqtt && typeof settings.mqtt === 'object') {
            projectConfig.mqtt = {
                ...projectConfig.mqtt,
                ...settings.mqtt
            };
        }

        if (Object.prototype.hasOwnProperty.call(settings, 'alert_cooldown')) {
            const cooldown = Number.parseInt(settings.alert_cooldown, 10);
            if (Number.isFinite(cooldown) && cooldown >= 0) {
                projectConfig.alert_cooldown = cooldown;
            }
        }

        if (Object.prototype.hasOwnProperty.call(settings, 'model_path') && typeof settings.model_path === 'string') {
            projectConfig.model_path = settings.model_path;
        }
    }

    return saveConfig(projectConfig);
};

const updateStreams = (streams) => {
    loadConfig();
    if (!Array.isArray(streams)) return false;
    projectConfig.streams = streams;
    return saveConfig(projectConfig);
};

// Initial Load
loadConfig();

module.exports = {
    getConfig,
    saveConfig,
    updateGlobalSettings,
    updateStreams
};
