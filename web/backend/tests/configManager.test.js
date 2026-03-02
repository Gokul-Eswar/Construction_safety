const fs = require('fs');
const path = require('path');
const configManager = require('../src/configManager');

// Mock fs
jest.mock('fs');

describe('ConfigManager', () => {
    const mockConfig = {
        mqtt: { host: 'localhost', port: 1883 },
        streams: [{ id: 'test', uri: 'rtsp://test' }]
    };

    beforeEach(() => {
        jest.clearAllMocks();
    });

    test('getConfig should return parsed config when file exists', () => {
        fs.existsSync.mockReturnValue(true);
        fs.readFileSync.mockReturnValue(JSON.stringify(mockConfig));

        const config = configManager.getConfig();
        expect(config).toEqual(mockConfig);
        expect(fs.readFileSync).toHaveBeenCalled();
    });

    test('getConfig should return empty object when file does not exist', () => {
        fs.existsSync.mockReturnValue(false);

        const config = configManager.getConfig();
        expect(config).toEqual({});
    });

    test('saveConfig should write stringified JSON to file', () => {
        fs.writeFileSync.mockReturnValue(true);

        const result = configManager.saveConfig(mockConfig);
        expect(result).toBe(true);
        expect(fs.writeFileSync).toHaveBeenCalledWith(
            expect.any(String),
            expect.stringContaining('"id": "test"')
        );
    });

    test('updateGlobalSettings should merge settings and save', () => {
        fs.existsSync.mockReturnValue(true);
        fs.readFileSync.mockReturnValue(JSON.stringify(mockConfig));
        
        const newSettings = { mqtt: { host: 'newhost' }, model_path: 'new/path.onnx' };
        configManager.updateGlobalSettings(newSettings);

        expect(fs.writeFileSync).toHaveBeenCalledWith(
            expect.any(String),
            expect.stringContaining('"host": "newhost"')
        );
    });
});
