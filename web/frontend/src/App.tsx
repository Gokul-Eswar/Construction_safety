import { useEffect, useState } from 'react';
import { AppBar, Toolbar, Typography, Container, Grid, Card, CardContent, Box } from '@mui/material';
import { DataGrid, GridColDef } from '@mui/x-data-grid';
import axios from 'axios';
import WarningIcon from '@mui/icons-material/Warning';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';

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
    fetchData();
    const interval = setInterval(fetchData, 5000); // Poll every 5 seconds
    return () => clearInterval(interval);
  }, []);

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
        </Toolbar>
      </AppBar>

      <Container maxWidth="lg" sx={{ mt: 4 }}>
        <Grid container spacing={3}>
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
      </Container>
    </Box>
  );
}

export default App;
