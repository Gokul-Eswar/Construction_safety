const express = require('express');
const cors = require('cors');
const path = require('path');
const http = require('http');
const { Server } = require('socket.io');

const configManager = require('./src/configManager');
const mqttService = require('./src/mqttService');
const apiRoutes = require('./src/routes');

const app = express();
const PORT = 3001;

app.use(cors());
app.use(express.json());

// Basic Auth Middleware
const authMiddleware = (req, res, next) => {
    const config = configManager.getConfig();
    if (!config.auth || !config.auth.username) return next();

    const b64auth = (req.headers.authorization || '').split(' ')[1] || '';
    const [login, password] = Buffer.from(b64auth, 'base64').toString().split(':');

    if (login && password && login === config.auth.username && password === config.auth.password) {
        return next();
    }

    res.set('WWW-Authenticate', 'Basic realm="Sentinel Safety Dashboard"');
    res.status(401).send('Authentication required.');
};

// Protect API
app.use('/api', authMiddleware);

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

// Mount Routes
app.use('/api', apiRoutes);

// Socket.IO Connection Events
io.on('connection', (socket) => {
    console.log('New client connected');
    socket.on('disconnect', () => console.log('Client disconnected'));
});

// Serve Frontend in Production
if (process.env.NODE_ENV === 'production') {
    app.use(express.static(path.join(__dirname, '../frontend/dist')));
    
    app.get('*', (req, res) => {
        res.sendFile(path.join(__dirname, '../frontend/dist/index.html'));
    });
}

server.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});