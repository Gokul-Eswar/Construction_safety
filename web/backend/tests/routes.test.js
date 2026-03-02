const request = require('supertest');
const app = require('../app');
const db = require('../db');
const configManager = require('../src/configManager');
const mqttService = require('../src/mqttService');

// Mock dependencies
jest.mock('../db');
jest.mock('../src/configManager');
jest.mock('../src/mqttService');

describe('API Routes', () => {
    beforeEach(() => {
        jest.clearAllMocks();
    });

    describe('GET /api/stats', () => {
        test('should return system stats', async () => {
            db.get.mockImplementation((sql, params, callback) => {
                callback(null, { count: 5 });
            });
            configManager.getConfig.mockReturnValue({ streams: [{}, {}] });

            const res = await request(app).get('/api/stats');
            
            expect(res.statusCode).toBe(200);
            expect(res.body).toEqual({
                today_violations: 5,
                system_status: 'online',
                active_streams: 2
            });
        });

        test('should handle DB errors', async () => {
            db.get.mockImplementation((sql, params, callback) => {
                callback(new Error('DB Error'));
            });

            const res = await request(app).get('/api/stats');
            expect(res.statusCode).toBe(500);
            expect(res.body.error).toBe('DB Error');
        });
    });

    describe('GET /api/config', () => {
        test('should return filtered config', async () => {
            configManager.getConfig.mockReturnValue({
                mqtt: { password: 'secret' },
                streams: [{ uri: 'rtsp://user:pass@host/path' }]
            });

            const res = await request(app).get('/api/config');
            
            expect(res.statusCode).toBe(200);
            expect(res.body.mqtt.password).toBe('***');
            expect(res.body.streams[0].uri).toBe('rtsp://***:***@host/path');
        });
    });

    describe('POST /api/system/restart', () => {
        test('should trigger restart', async () => {
            mqttService.sendRestartCommand.mockResolvedValue(true);

            const res = await request(app).post('/api/system/restart');
            
            expect(res.statusCode).toBe(200);
            expect(res.body.success).toBe(true);
            expect(mqttService.sendRestartCommand).toHaveBeenCalled();
        });
    });
});
