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
    console.log('New client connected');
    socket.on('disconnect', () => console.log('Client disconnected'));
});

server.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});
