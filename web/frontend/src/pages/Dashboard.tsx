import { useEffect, useState, useRef } from 'react';
import { Grid, Card, CardContent, Typography, Box, CircularProgress, Chip, Alert, Snackbar, Table, TableBody, TableCell, TableContainer, TableHead, TableRow, Paper } from '@mui/material';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';
import axios from 'axios';
import io, { Socket } from 'socket.io-client';
import MJPEGPlayer from '../components/MJPEGPlayer';

interface Stats {
  today_violations: number;
  system_status: string;
  active_streams: number;
}

export default function Dashboard() {
  const [stats, setStats] = useState<Stats | null>(null);
  const [loading, setLoading] = useState(true);
  const [recentAlert, setRecentAlert] = useState<string | null>(null);
  const [systemOnline, setSystemOnline] = useState(false);
  const [telemetry, setTelemetry] = useState<any>(null);
  const [lastSyncTime, setLastSyncTime] = useState<Date | null>(null);
  const lastHeartbeat = useRef<number>(0);
  const socketRef = useRef<Socket | null>(null);

  useEffect(() => {
    // Initialize Socket.IO connection inside the component
    console.log('🔌 [Dashboard] Initializing Socket.IO connection to http://localhost:3001');
    
    const socket = io('http://localhost:3001', {
        reconnection: true,
        reconnectionDelay: 1000,
        reconnectionDelayMax: 5000,
        reconnectionAttempts: 5
    });
    
    socketRef.current = socket;

    const fetchStats = async () => {
      try {
        const res = await axios.get('/api/stats');
        setStats(res.data);
      } catch (err) {
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    
    fetchStats();
    const interval = setInterval(fetchStats, 10000); 

    // Watchdog for Heartbeat
    const watchdog = setInterval(() => {
        if (Date.now() - lastHeartbeat.current > 5000) {
            setSystemOnline(false);
        }
    }, 1000);

    // Socket.IO Connection Events
    socket.on('connect', () => {
        console.log('✓ [Socket.IO] Connected - Client ID:', socket.id);
    });

    socket.on('connect_error', (error) => {
        console.error('✗ [Socket.IO] Connection Error:', error);
    });

    socket.on('disconnect', () => {
        console.log('✗ [Socket.IO] Disconnected');
    });

    // Socket.IO Message Listeners
    socket.on('violation_alert', (data: any) => {
        console.log('[Socket.IO] Real-time Alert:', data);
        setRecentAlert(`Zone Violation: ${data.zone_name || 'Unknown Zone'}`);
        setStats(prev => prev ? { ...prev, today_violations: prev.today_violations + 1 } : null);
    });

    socket.on('system_heartbeat', (data: any) => {
        console.log('[Socket.IO] ♥ Heartbeat received:', data);
        lastHeartbeat.current = Date.now();
        setSystemOnline(true);
    });

    socket.on('system_telemetry', (data: any) => {
        console.log('[Socket.IO] Telemetry:', data);
        setTelemetry(data);
    });

    socket.on('cloud_sync_event', (data: any) => {
        console.log('[Socket.IO] Cloud sync event:', data);
        setLastSyncTime(new Date());
    });

    return () => {
        clearInterval(interval);
        clearInterval(watchdog);
        if (socket) socket.disconnect();
        console.log('🔌 [Dashboard] Cleanup - Socket.IO disconnected');
    };
  }, []);

  if (loading) return <Box display="flex" justifyContent="center"><CircularProgress /></Box>;

    const latencyP99Values = telemetry?.latency
        ? Object.values(telemetry.latency)
                .map((metric: any) => Number(metric?.p99))
                .filter((value: number) => Number.isFinite(value))
        : [];
    const aggregateP99 = latencyP99Values.length > 0 ? Math.max(...latencyP99Values) : 0;

  return (
    <Grid container spacing={3}>
      {!systemOnline && (
          <Grid item xs={12}>
              <Alert severity="error" variant="filled" icon={<VideocamOffIcon fontSize="inherit" />}>
                  <Typography variant="subtitle1" fontWeight="bold">
                      SYSTEM OFFLINE
                  </Typography>
                  The inference engine is not reachable. Live streams and alerts may be unavailable.
              </Alert>
          </Grid>
      )}

      <Grid item xs={12} sm={6} md={3}>
          <Card>
              <CardContent>
                  <Typography color="text.secondary" gutterBottom>System Health</Typography>
                  <Box display="flex" alignItems="center">
                      <CheckCircleIcon sx={{ mr: 1, color: systemOnline ? 'green' : 'gray' }} />
                      <Typography variant="h4">{systemOnline ? 'Online' : 'Offline'}</Typography>
                  </Box>
              </CardContent>
          </Card>
      </Grid>

      <Grid item xs={12} sm={6} md={3}>
          <Card>
              <CardContent>
                  <Typography color="text.secondary" gutterBottom>Active Cameras</Typography>
                  <Typography variant="h4">{stats?.active_streams || 0}</Typography>
              </CardContent>
          </Card>
      </Grid>

      <Grid item xs={12} sm={6} md={3}>
          <Card>
              <CardContent>
                  <Typography color="text.secondary" gutterBottom>Violations Today</Typography>
                  <Typography variant="h4" color="error">{stats?.today_violations || 0}</Typography>
              </CardContent>
          </Card>
      </Grid>

      <Grid item xs={12} sm={6} md={3}>
          <Card>
              <CardContent>
                  <Typography color="text.secondary" gutterBottom>Status</Typography>
                  <Chip 
                      label={systemOnline ? 'LIVE' : 'OFFLINE'} 
                      color={systemOnline ? 'success' : 'default'}
                      variant="outlined"
                  />
              </CardContent>
          </Card>
      </Grid>

      <Grid item xs={12}>
          <Card>
              <CardContent>
                  <Typography variant="h6" gutterBottom>Live Feed</Typography>
                  <Box sx={{ backgroundColor: '#000', minHeight: 400, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
                      <Typography color="textSecondary">No camera selected</Typography>
                  </Box>
              </CardContent>
          </Card>
      </Grid>

      <Snackbar
          open={!!recentAlert}
          autoHideDuration={6000}
          onClose={() => setRecentAlert(null)}
      >
          <Alert onClose={() => setRecentAlert(null)} severity="warning">
              {recentAlert}
          </Alert>
      </Snackbar>
    </Grid>
  );
}
