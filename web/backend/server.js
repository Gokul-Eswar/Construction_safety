const http = require('http');
const { Server } = require('socket.io');
const app = require('./app');
const mqttService = require('./src/mqttService');

const PORT = process.env.PORT || 3001;

// Setup Socket.IO
const server = http.createServer(app);
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

// Initialize MQTT Service with Socket.IO instance
mqttService.setup(io);

// Socket.IO Connection Events
io.on('connection', (socket) => {
    console.log(`[Socket.IO] ✓ Client connected: ${socket.id}`);
    
    socket.on('disconnect', () => {
        console.log(`[Socket.IO] ✗ Client disconnected: ${socket.id}`);
    });
});

// Setup a global event listener to verify emissions
const originalEmit = io.emit.bind(io);
io.emit = function(eventName, ...args) {
    if (eventName === 'system_heartbeat' || eventName === 'violation_alert' || eventName === 'system_telemetry') {
        console.log(`[Socket.IO EMIT] Event: ${eventName}, Clients: ${io.engine.clientsCount}`);
    }
    return originalEmit(eventName, ...args);
};

server.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});
