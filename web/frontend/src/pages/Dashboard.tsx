import { useEffect, useState, useRef } from 'react';
import { Grid, Card, CardContent, Typography, Box, CircularProgress, Chip, Alert, Snackbar, Table, TableBody, TableCell, TableContainer, TableHead, TableRow, Paper, Divider } from '@mui/material';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';
import axios from 'axios';
import io from 'socket.io-client';
import MJPEGPlayer from '../components/MJPEGPlayer';

interface Stats {
  today_violations: number;
  system_status: string;
  active_streams: number;
}

const socket = io(); // Connects to same host/port by default in prod

export default function Dashboard() {
  const [stats, setStats] = useState<Stats | null>(null);
  const [loading, setLoading] = useState(true);
  const [recentAlert, setRecentAlert] = useState<string | null>(null);
  const [systemOnline, setSystemOnline] = useState(false); // Default false until heartbeat
  const [telemetry, setTelemetry] = useState<any>(null);
  const lastHeartbeat = useRef<number>(0);

  useEffect(() => {
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
    // Keep polling for stats as backup/sync, but slower
    const interval = setInterval(fetchStats, 10000); 

    // Watchdog for Heartbeat
    const watchdog = setInterval(() => {
        if (Date.now() - lastHeartbeat.current > 5000) {
            setSystemOnline(false);
        }
    }, 1000);

    // Socket.IO Listeners
    socket.on('connect', () => {
        console.log('Connected to WebSocket');
    });

    socket.on('violation_alert', (data: any) => {
        console.log('Real-time Alert:', data);
        setRecentAlert(`Zone Violation: ${data.zone_name || 'Unknown Zone'}`);
        // Increment stats locally for instant feedback
        setStats(prev => prev ? { ...prev, today_violations: prev.today_violations + 1 } : null);
    });

    socket.on('system_heartbeat', () => {
        lastHeartbeat.current = Date.now();
        setSystemOnline(true);
    });

    socket.on('system_telemetry', (data: any) => {
        setTelemetry(data);
    });

    return () => {
        clearInterval(interval);
        clearInterval(watchdog);
        socket.off('connect');
        socket.off('violation_alert');
        socket.off('system_heartbeat');
        socket.off('system_telemetry');
    };
  }, []);

  if (loading) return <Box display="flex" justifyContent="center"><CircularProgress /></Box>;

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

      <Snackbar open={!!recentAlert} autoHideDuration={6000} onClose={() => setRecentAlert(null)} anchorOrigin={{ vertical: 'top', horizontal: 'center' }}>
        <Alert severity="error" variant="filled" sx={{ width: '100%' }}>
          {recentAlert}
        </Alert>
      </Snackbar>

      {/* Metrics Row */}
      <Grid item xs={12} md={4}>
        <Card sx={{ height: '100%', bgcolor: 'background.paper' }}>
          <CardContent>
            <Typography color="text.secondary" gutterBottom>System Health</Typography>
            <Box display="flex" alignItems="center" gap={1}>
              <CheckCircleIcon color="success" fontSize="large" />
              <Typography variant="h4">{stats?.system_status || 'Unknown'}</Typography>
            </Box>
          </CardContent>
        </Card>
      </Grid>
      <Grid item xs={12} md={4}>
        <Card sx={{ height: '100%' }}>
          <CardContent>
             <Typography color="text.secondary" gutterBottom>Active Cameras</Typography>
             <Typography variant="h4">{stats?.active_streams || 0}</Typography>
          </CardContent>
        </Card>
      </Grid>
      <Grid item xs={12} md={4}>
        <Card sx={{ height: '100%' }}>
          <CardContent>
             <Typography color="text.secondary" gutterBottom>Violations Today</Typography>
             <Typography variant="h4" color="error">{stats?.today_violations || 0}</Typography>
          </CardContent>
        </Card>
      </Grid>

      {/* Live Feed Section */}
      <Grid item xs={12}>
        <Card>
          <CardContent>
            <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
              <Typography variant="h6">Live Surveillance Grid</Typography>
              <Box display="flex" gap={2}>
                {telemetry && Object.entries(telemetry.streams || {}).map(([key, val]: any) => (
                    <Chip 
                        key={key} 
                        label={`${key}: ${val.fps.toFixed(1)} FPS`} 
                        color={val.active ? "success" : "default"} 
                        variant="outlined" 
                        size="small" 
                    />
                ))}
                {telemetry && telemetry.latency && (
                    <Chip 
                        label={`Latency (P99): ${
                            Object.values(telemetry.latency).find((l: any) => l.p99) 
                            ? (Object.values(telemetry.latency)[0] as any).p99.toFixed(1) 
                            : '0.0'
                        }ms`} 
                        color="warning" 
                        variant="outlined" 
                        size="small" 
                    />
                )}
                <Chip icon={<CheckCircleIcon />} label="Live" color="success" size="small" variant="outlined" />
              </Box>
            </Box>
            
            <MJPEGPlayer 
                url={`http://${window.location.hostname}:8081`} 
                label="Primary Site Camera"
            />
          </CardContent>
        </Card>
      </Grid>

      {/* Detailed Metrics Section */}
      <Grid item xs={12}>
        <Card>
          <CardContent>
             <Typography variant="h6" gutterBottom>System Metrics</Typography>
             <Typography variant="body2" color="text.secondary" paragraph>
                Real-time performance statistics from the inference engine.
             </Typography>
             
             <Grid container spacing={3}>
                {/* Operation Stats */}
                <Grid item xs={12} md={6}>
                    <Typography variant="subtitle1" sx={{ fontWeight: 'bold', mb: 1 }}>Operation Statistics</Typography>
                    <TableContainer component={Paper} variant="outlined">
                        <Table size="small">
                            <TableHead>
                                <TableRow>
                                    <TableCell>Stream ID</TableCell>
                                    <TableCell align="right">Status</TableCell>
                                    <TableCell align="right">FPS</TableCell>
                                    <TableCell align="right">Frames Processed</TableCell>
                                </TableRow>
                            </TableHead>
                            <TableBody>
                                {telemetry && telemetry.streams ? (
                                    Object.entries(telemetry.streams).map(([key, val]: any) => (
                                        <TableRow key={key}>
                                            <TableCell component="th" scope="row">{key}</TableCell>
                                            <TableCell align="right">
                                                <Chip 
                                                    label={val.active ? "Active" : "Inactive"} 
                                                    color={val.active ? "success" : "error"} 
                                                    size="small" 
                                                    sx={{ height: 20, fontSize: '0.7rem' }}
                                                />
                                            </TableCell>
                                            <TableCell align="right">{val.fps.toFixed(1)}</TableCell>
                                            <TableCell align="right">{val.frame_count.toLocaleString()}</TableCell>
                                        </TableRow>
                                    ))
                                ) : (
                                    <TableRow>
                                        <TableCell colSpan={4} align="center">No telemetry data</TableCell>
                                    </TableRow>
                                )}
                            </TableBody>
                        </Table>
                    </TableContainer>
                </Grid>

                {/* Model Stats */}
                <Grid item xs={12} md={6}>
                    <Typography variant="subtitle1" sx={{ fontWeight: 'bold', mb: 1 }}>Model Latency (ms)</Typography>
                    <TableContainer component={Paper} variant="outlined">
                        <Table size="small">
                            <TableHead>
                                <TableRow>
                                    <TableCell>Component</TableCell>
                                    <TableCell align="right">Avg</TableCell>
                                    <TableCell align="right">Min</TableCell>
                                    <TableCell align="right">Max</TableCell>
                                    <TableCell align="right">P99</TableCell>
                                </TableRow>
                            </TableHead>
                            <TableBody>
                                {telemetry && telemetry.latency ? (
                                    Object.entries(telemetry.latency).map(([key, val]: any) => {
                                        // Clean up key name for display (e.g. "cam_01_inference" -> "Inference")
                                        const parts = key.split('_');
                                        const label = parts[parts.length - 1].charAt(0).toUpperCase() + parts[parts.length - 1].slice(1);
                                        const stream = parts.slice(0, -1).join('_');

                                        return (
                                            <TableRow key={key}>
                                                <TableCell component="th" scope="row">
                                                    {label} <Typography variant="caption" color="text.secondary">({stream})</Typography>
                                                </TableCell>
                                                <TableCell align="right">{val.avg.toFixed(2)}</TableCell>
                                                <TableCell align="right">{val.min.toFixed(2)}</TableCell>
                                                <TableCell align="right">{val.max.toFixed(2)}</TableCell>
                                                <TableCell align="right" sx={{ fontWeight: 'bold' }}>{val.p99.toFixed(2)}</TableCell>
                                            </TableRow>
                                        );
                                    })
                                ) : (
                                    <TableRow>
                                        <TableCell colSpan={5} align="center">No latency data</TableCell>
                                    </TableRow>
                                )}
                            </TableBody>
                        </Table>
                    </TableContainer>
                </Grid>
             </Grid>
          </CardContent>
        </Card>
      </Grid>
    </Grid>
  );
}
