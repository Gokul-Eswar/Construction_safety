import { useEffect, useState } from 'react';
import { AppBar, Toolbar, Typography, Container, Grid, Card, CardContent, Box, Tab, Tabs } from '@mui/material';
import { DataGrid, GridColDef } from '@mui/x-data-grid';
import axios from 'axios';
import WarningIcon from '@mui/icons-material/Warning';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import ZoneEditor from './ZoneEditor';

interface Violation {
  id: number;
  timestamp: string;
  zone_id: number;
  zone_name: string;
  confidence: number;
}

interface Stats {
  today_violations: number;
  system_status: string;
}

function App() {
  const [tabIndex, setTabIndex] = useState(0);
  const [violations, setViolations] = useState<Violation[]>([]);
  const [stats, setStats] = useState<Stats | null>(null);

  const fetchData = async () => {
    try {
      const vRes = await axios.get('/api/violations?limit=20');
      setViolations(vRes.data.data);

      const sRes = await axios.get('/api/stats');
      setStats(sRes.data);
    } catch (err) {
      console.error(err);
    }
  };

  useEffect(() => {
    if (tabIndex === 0) {
      fetchData();
      const interval = setInterval(fetchData, 5000); // Poll every 5 seconds
      return () => clearInterval(interval);
    }
  }, [tabIndex]);

  const columns: GridColDef[] = [
    { field: 'id', headerName: 'ID', width: 70 },
    { field: 'timestamp', headerName: 'Timestamp', width: 200 },
    { field: 'zone_name', headerName: 'Zone', width: 200 },
    { 
      field: 'confidence', 
      headerName: 'Confidence', 
      width: 130,
      valueFormatter: (params: any) => `${(params.value * 100).toFixed(1)}%`
    },
  ];

  return (
    <Box sx={{ flexGrow: 1 }}>
      <AppBar position="static">
        <Toolbar>
          <WarningIcon sx={{ mr: 2 }} />
          <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
            Construction Safety Monitor
          </Typography>
          <Tabs value={tabIndex} onChange={(_, val) => setTabIndex(val)} textColor="inherit" indicatorColor="secondary">
            <Tab label="Monitor" />
            <Tab label="Zone Editor" />
          </Tabs>
        </Toolbar>
      </AppBar>

      <Container maxWidth="lg" sx={{ mt: 4 }}>
        {tabIndex === 0 ? (
          <Grid container spacing={3}>
            {/* Live Video Feed */}
            <Grid item xs={12}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>Live Video Feed</Typography>
                  <Box 
                    display="flex" 
                    justifyContent="center" 
                    bgcolor="#000" 
                    borderRadius={1} 
                    overflow="hidden"
                    sx={{ minHeight: 400 }}
                  >
                    <img 
                      src="http://localhost:8081" 
                      alt="Live Stream" 
                      style={{ maxWidth: '100%', maxHeight: '600px', display: 'block' }} 
                      onError={(e: any) => {
                        e.target.style.display = 'none';
                        e.target.nextSibling.style.display = 'block';
                      }}
                    />
                    <Box 
                      display="none" 
                      sx={{ color: '#fff', p: 4, textAlign: 'center' }}
                    >
                      <Typography variant="h6">Live Feed Offline</Typography>
                      <Typography variant="body2" color="grey.500">
                        Ensure the Sentinel Engine is running and port 8081 is accessible.
                      </Typography>
                    </Box>
                  </Box>
                </CardContent>
              </Card>
            </Grid>

            {/* Stats Cards */}
            <Grid item xs={12} md={6}>
              <Card>
                <CardContent>
                  <Typography color="text.secondary" gutterBottom>
                    System Status
                  </Typography>
                  <Box display="flex" alignItems="center">
                    <CheckCircleIcon color="success" sx={{ mr: 1 }} />
                    <Typography variant="h5">
                      {stats?.system_status || 'Offline'}
                    </Typography>
                  </Box>
                </CardContent>
              </Card>
            </Grid>
            <Grid item xs={12} md={6}>
              <Card>
                <CardContent>
                  <Typography color="text.secondary" gutterBottom>
                    Violations Today
                  </Typography>
                  <Typography variant="h5" color="error">
                    {stats?.today_violations || 0}
                  </Typography>
                </CardContent>
              </Card>
            </Grid>

            {/* Data Table */}
            <Grid item xs={12}>
              <Typography variant="h5" gutterBottom sx={{ mt: 2 }}>
                Recent Violations
              </Typography>
              <div style={{ height: 400, width: '100%' }}>
                <DataGrid
                  rows={violations}
                  columns={columns}
                  initialState={{
                    pagination: {
                      paginationModel: { page: 0, pageSize: 5 },
                    },
                  }}
                  pageSizeOptions={[5, 10]}
                  disableRowSelectionOnClick
                />
              </div>
            </Grid>
          </Grid>
        ) : (
          <ZoneEditor />
        )}
      </Container>
    </Box>
  );
}

export default App;
