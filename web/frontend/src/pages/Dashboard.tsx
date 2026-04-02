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

    const streamTelemetry = telemetry?.streams ? Object.entries(telemetry.streams) : [];
    const latencyTelemetry = telemetry?.latency ? Object.entries(telemetry.latency) : [];
    const gpuTelemetry = telemetry?.gpu || null;

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
        const formatMetricLabel = (key: string) => key.replace(/_/g, ' ').replace(/\b\w/g, char => char.toUpperCase());

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
                  <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2, flexWrap: 'wrap', gap: 1 }}>
                      <Typography variant="h6">Live Surveillance Grid</Typography>
                      <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap' }}>
                          {streamTelemetry.map(([streamId, stream]: any) => (
                              <Chip
                                  key={streamId}
                                  label={`${streamId}: ${Number(stream?.fps || 0).toFixed(1)} FPS`}
                                  color={stream?.active ? 'success' : 'default'}
                                  variant="outlined"
                                  size="small"
                              />
                          ))}
                          {telemetry?.latency && (
                              <Chip
                                  label={`Latency (P99): ${aggregateP99.toFixed(1)}ms`}
                                  color="warning"
                                  variant="outlined"
                                  size="small"
                              />
                          )}
                          <Chip
                              label={systemOnline ? 'Live' : 'Offline'}
                              color={systemOnline ? 'success' : 'default'}
                              variant="outlined"
                              size="small"
                          />
                      </Box>
                  </Box>
                  <MJPEGPlayer
                      url={`http://${window.location.hostname}:8081`}
                      label="Primary Site Camera"
                  />
              </CardContent>
          </Card>
      </Grid>

      <Grid item xs={12}>
          <Card>
              <CardContent>
                  <Typography variant="h6" gutterBottom>System Metrics</Typography>
                  <Typography variant="body2" color="text.secondary" paragraph>
                      Real-time performance statistics from the inference engine.
                  </Typography>

                  <Grid container spacing={3}>
                      <Grid item xs={12} md={4}>
                          <Typography variant="subtitle1" sx={{ fontWeight: 'bold', mb: 1 }}>
                              Operation Statistics
                          </Typography>
                          <TableContainer component={Paper} variant="outlined">
                              <Table size="small">
                                  <TableHead>
                                      <TableRow>
                                          <TableCell>Stream ID</TableCell>
                                          <TableCell align="right">FPS</TableCell>
                                          <TableCell align="right">Frames</TableCell>
                                      </TableRow>
                                  </TableHead>
                                  <TableBody>
                                      {streamTelemetry.length > 0 ? (
                                          streamTelemetry.map(([streamId, stream]: any) => (
                                              <TableRow key={streamId}>
                                                  <TableCell component="th" scope="row">
                                                      {streamId}
                                                  </TableCell>
                                                  <TableCell align="right">{Number(stream?.fps || 0).toFixed(1)}</TableCell>
                                                  <TableCell align="right">{Number(stream?.frame_count || 0).toLocaleString()}</TableCell>
                                              </TableRow>
                                          ))
                                      ) : (
                                          <TableRow>
                                              <TableCell colSpan={3} align="center">No stream telemetry</TableCell>
                                          </TableRow>
                                      )}
                                  </TableBody>
                              </Table>
                          </TableContainer>
                      </Grid>

                      <Grid item xs={12} md={4}>
                          <Typography variant="subtitle1" sx={{ fontWeight: 'bold', mb: 1 }}>
                              Latency Profile
                          </Typography>
                          <TableContainer component={Paper} variant="outlined">
                              <Table size="small">
                                  <TableHead>
                                      <TableRow>
                                          <TableCell>Stage</TableCell>
                                          <TableCell align="right">Avg</TableCell>
                                          <TableCell align="right">P99</TableCell>
                                      </TableRow>
                                  </TableHead>
                                  <TableBody>
                                      {latencyTelemetry.length > 0 ? (
                                          latencyTelemetry.map(([metricName, metric]: any) => (
                                              <TableRow key={metricName}>
                                                  <TableCell component="th" scope="row">
                                                      {formatMetricLabel(metricName)}
                                                  </TableCell>
                                                  <TableCell align="right">{Number(metric?.avg || 0).toFixed(1)}</TableCell>
                                                  <TableCell align="right" sx={{ fontWeight: 'bold' }}>{Number(metric?.p99 || 0).toFixed(1)}</TableCell>
                                              </TableRow>
                                          ))
                                      ) : (
                                          <TableRow>
                                              <TableCell colSpan={3} align="center">No latency data</TableCell>
                                          </TableRow>
                                      )}
                                  </TableBody>
                              </Table>
                          </TableContainer>
                      </Grid>

                      <Grid item xs={12} md={4}>
                          <Typography variant="subtitle1" sx={{ fontWeight: 'bold', mb: 1 }}>
                              GPU Infrastructure
                          </Typography>
                          <TableContainer component={Paper} variant="outlined">
                              <Table size="small">
                                  <TableHead>
                                      <TableRow>
                                          <TableCell>Metric</TableCell>
                                          <TableCell align="right">Value</TableCell>
                                      </TableRow>
                                  </TableHead>
                                  <TableBody>
                                      {gpuTelemetry ? (
                                          <>
                                              <TableRow>
                                                  <TableCell component="th" scope="row">Utilization</TableCell>
                                                  <TableCell align="right">
                                                      <Chip
                                                          label={`${Number(gpuTelemetry.utilization || 0)}%`}
                                                          color={Number(gpuTelemetry.utilization || 0) > 80 ? 'error' : 'primary'}
                                                          size="small"
                                                          sx={{ height: 20, fontSize: '0.75rem' }}
                                                      />
                                                  </TableCell>
                                              </TableRow>
                                              <TableRow>
                                                  <TableCell component="th" scope="row">Temperature</TableCell>
                                                  <TableCell align="right">{Number(gpuTelemetry.temperature || 0).toFixed(1)}°C</TableCell>
                                              </TableRow>
                                              <TableRow>
                                                  <TableCell component="th" scope="row">Memory Used</TableCell>
                                                  <TableCell align="right">{Number(gpuTelemetry.memory_used_mb || 0).toLocaleString()} MB</TableCell>
                                              </TableRow>
                                              <TableRow>
                                                  <TableCell component="th" scope="row">Memory Total</TableCell>
                                                  <TableCell align="right">{Number(gpuTelemetry.memory_total_mb || 0).toLocaleString()} MB</TableCell>
                                              </TableRow>
                                          </>
                                      ) : (
                                          <TableRow>
                                              <TableCell colSpan={2} align="center">No GPU metrics</TableCell>
                                          </TableRow>
                                      )}
                                  </TableBody>
                              </Table>
                          </TableContainer>
                      </Grid>
                  </Grid>

                  {lastSyncTime && (
                      <Typography variant="caption" color="text.secondary" sx={{ display: 'block', mt: 2 }}>
                          Last cloud sync: {lastSyncTime.toLocaleString()}
                      </Typography>
                  )}
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
