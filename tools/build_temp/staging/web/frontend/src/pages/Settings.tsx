import { useEffect, useState } from 'react';
import { 
    Box, Typography, Button, TextField, Paper, Grid, Alert, CircularProgress 
} from '@mui/material';
import SaveIcon from '@mui/icons-material/Save';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import axios from 'axios';

interface MQTTConfig {
    host: string;
    port: number;
    topic: string;
    client_id: string;
}

interface GlobalConfig {
    mqtt: MQTTConfig;
    alert_cooldown: number;
    model_path: string;
}

export default function Settings() {
    const [config, setConfig] = useState<GlobalConfig | null>(null);
    const [loading, setLoading] = useState(true);
    const [message, setMessage] = useState<{ type: 'success' | 'error', text: string } | null>(null);

    useEffect(() => {
        axios.get('/api/config')
            .then(res => setConfig(res.data))
            .catch(err => console.error(err))
            .finally(() => setLoading(false));
    }, []);

    const handleChange = (field: string, value: any) => {
        if (!config) return;
        setConfig({ ...config, [field]: value });
    };

    const handleMQTTChange = (field: string, value: any) => {
        if (!config) return;
        setConfig({ ...config, mqtt: { ...config.mqtt, [field]: value } });
    };

    const handleSave = async () => {
        if (!config) return;
        try {
            await axios.post('/api/config/global', {
                mqtt: config.mqtt,
                alert_cooldown: config.alert_cooldown,
                model_path: config.model_path
            });
            setMessage({ type: 'success', text: 'Settings saved successfully!' });
        } catch (err) {
            setMessage({ type: 'error', text: 'Failed to save settings.' });
        }
    };

    const handleRestart = async () => {
        if (!confirm("This will restart the system service. Are you sure?")) return;
        try {
            await axios.post('/api/system/restart');
            setMessage({ type: 'success', text: 'Restart signal sent.' });
        } catch (err) {
            setMessage({ type: 'error', text: 'Failed to send restart signal.' });
        }
    };

    if (loading) return <CircularProgress />;

    return (
        <Box maxWidth="800px">
            <Typography variant="h5" gutterBottom>System Settings</Typography>
            
            {message && (
                <Alert severity={message.type} sx={{ mb: 2 }} onClose={() => setMessage(null)}>
                    {message.text}
                </Alert>
            )}

            <Paper sx={{ p: 3, mb: 3 }}>
                <Typography variant="h6" gutterBottom>Detection Parameters</Typography>
                <Grid container spacing={2}>
                    <Grid item xs={12} md={6}>
                        <TextField 
                            label="Model Path" 
                            fullWidth 
                            value={config?.model_path || ''} 
                            onChange={(e) => handleChange('model_path', e.target.value)}
                            helperText="Path to ONNX model or TensorRT engine"
                        />
                    </Grid>
                    <Grid item xs={12} md={6}>
                        <TextField 
                            label="Alert Cooldown (ms)" 
                            type="number"
                            fullWidth 
                            value={config?.alert_cooldown || 0} 
                            onChange={(e) => handleChange('alert_cooldown', parseInt(e.target.value))}
                            helperText="Time before re-alerting on same target"
                        />
                    </Grid>
                </Grid>
            </Paper>

            <Paper sx={{ p: 3, mb: 3 }}>
                <Typography variant="h6" gutterBottom>MQTT Integration</Typography>
                <Grid container spacing={2}>
                    <Grid item xs={12} md={8}>
                        <TextField 
                            label="Broker Host" 
                            fullWidth 
                            value={config?.mqtt.host || ''} 
                            onChange={(e) => handleMQTTChange('host', e.target.value)}
                        />
                    </Grid>
                    <Grid item xs={12} md={4}>
                        <TextField 
                            label="Port" 
                            type="number"
                            fullWidth 
                            value={config?.mqtt.port || 1883} 
                            onChange={(e) => handleMQTTChange('port', parseInt(e.target.value))}
                        />
                    </Grid>
                    <Grid item xs={12} md={6}>
                        <TextField 
                            label="Topic" 
                            fullWidth 
                            value={config?.mqtt.topic || ''} 
                            onChange={(e) => handleMQTTChange('topic', e.target.value)}
                        />
                    </Grid>
                    <Grid item xs={12} md={6}>
                        <TextField 
                            label="Client ID" 
                            fullWidth 
                            value={config?.mqtt.client_id || ''} 
                            onChange={(e) => handleMQTTChange('client_id', e.target.value)}
                        />
                    </Grid>
                </Grid>
            </Paper>

            <Box display="flex" justifyContent="space-between">
                <Button 
                    variant="outlined" 
                    color="error" 
                    startIcon={<RestartAltIcon />}
                    onClick={handleRestart}
                >
                    Restart Service
                </Button>
                <Button 
                    variant="contained" 
                    startIcon={<SaveIcon />}
                    onClick={handleSave}
                >
                    Save Changes
                </Button>
            </Box>
        </Box>
    );
}
