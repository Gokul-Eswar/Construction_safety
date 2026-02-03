const fs = require('fs');
const path = require('path');

const configPath = path.resolve(__dirname, '../../../config.json');
let projectConfig = {};

const loadConfig = () => {
    try {
        const rawData = fs.readFileSync(configPath);
        projectConfig = JSON.parse(rawData);
        return projectConfig;
    } catch (error) {
        console.error("Could not load config.json:", error);
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
    if (settings.mqtt) projectConfig.mqtt = settings.mqtt;
    if (settings.alert_cooldown) projectConfig.alert_cooldown = parseInt(settings.alert_cooldown);
    if (settings.model_path) projectConfig.model_path = settings.model_path;
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
