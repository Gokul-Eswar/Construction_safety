import { useEffect, useState } from 'react';
import { Grid, Card, CardContent, Typography, Box, CircularProgress, Chip } from '@mui/material';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';
import axios from 'axios';

interface Stats {
  today_violations: number;
  system_status: string;
  active_streams: number;
}

export default function Dashboard() {
  const [stats, setStats] = useState<Stats | null>(null);
  const [loading, setLoading] = useState(true);

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
    const interval = setInterval(fetchStats, 5000);
    return () => clearInterval(interval);
  }, []);

  if (loading) return <Box display="flex" justifyContent="center"><CircularProgress /></Box>;

  return (
    <Grid container spacing={3}>
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
            
            <Box 
              display="flex" 
              justifyContent="center" 
              alignItems="center"
              bgcolor="#000" 
              borderRadius={1} 
              overflow="hidden"
              sx={{ minHeight: 480, aspectRatio: '16/9', position: 'relative' }}
            >
              <img 
                src={`http://${window.location.hostname}:8081`}
                alt="Live Stream" 
                style={{ width: '100%', height: '100%', objectFit: 'contain' }} 
                onError={(e: any) => {
                  e.target.style.display = 'none';
                  e.target.nextSibling.style.display = 'flex';
                }}
              />
              <Box 
                display="none" 
                flexDirection="column"
                alignItems="center"
                justifyContent="center"
                sx={{ position: 'absolute', top: 0, left: 0, width: '100%', height: '100%', color: 'grey.700' }}
              >
                <VideocamOffIcon sx={{ fontSize: 60, mb: 2 }} />
                <Typography variant="h6">Stream Offline</Typography>
                <Typography variant="body2">Waiting for connection...</Typography>
              </Box>
            </Box>
          </CardContent>
        </Card>
      </Grid>
    </Grid>
  );
}
