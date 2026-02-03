import { useEffect, useState } from 'react';
import { Grid, Card, CardContent, Typography, Box, CircularProgress, Chip, Alert, Snackbar } from '@mui/material';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
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

    return () => {
        clearInterval(interval);
        socket.off('connect');
        socket.off('violation_alert');
    };
  }, []);

  if (loading) return <Box display="flex" justifyContent="center"><CircularProgress /></Box>;

  return (
    <Grid container spacing={3}>
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
              <Chip icon={<CheckCircleIcon />} label="Live" color="success" size="small" variant="outlined" />
            </Box>
            
            <MJPEGPlayer 
                url={`http://${window.location.hostname}:8081`} 
                label="Primary Site Camera"
            />
          </CardContent>
        </Card>
      </Grid>
    </Grid>
  );
}
