const express = require('express');
const cors = require('cors');
const compression = require('compression');
const path = require('path');
const crypto = require('crypto');

const configManager = require('./src/configManager');
const apiRoutes = require('./src/routes');

const app = express();

app.use(compression());
app.use(cors());
app.use(express.json());

const safeEqual = (left, right) => {
    if (typeof left !== 'string' || typeof right !== 'string') return false;
    const leftBuf = Buffer.from(left, 'utf8');
    const rightBuf = Buffer.from(right, 'utf8');
    if (leftBuf.length !== rightBuf.length) return false;
    return crypto.timingSafeEqual(leftBuf, rightBuf);
};

// Basic Auth Middleware
const authMiddleware = (req, res, next) => {
    const config = configManager.getConfig();
    const authUsername = process.env.API_AUTH_USERNAME || config?.auth?.username;
    const authPassword = process.env.API_AUTH_PASSWORD || config?.auth?.password;

    if (!authUsername) return next();

    const b64auth = (req.headers.authorization || '').split(' ')[1] || '';
    const [login = '', password = ''] = Buffer.from(b64auth, 'base64').toString().split(':');

    if (safeEqual(login, authUsername) && safeEqual(password, authPassword || '')) {
        return next();
    }

    res.set('WWW-Authenticate', 'Basic realm="Sentinel Safety Dashboard"');
    res.status(401).send('Authentication required.');
};

// Protect API
app.use('/api', authMiddleware);

// Mount Routes
app.use('/api', apiRoutes);

// Serve Frontend in Production
if (process.env.NODE_ENV === 'production') {
    app.use(express.static(path.join(__dirname, '../frontend/dist')));
    
    app.get('*', (req, res) => {
        res.sendFile(path.join(__dirname, '../frontend/dist/index.html'));
    });
}

module.exports = app;
