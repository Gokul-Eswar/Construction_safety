import { useState } from 'react';
import { ThemeProvider } from '@mui/material/styles';
import { theme } from './theme';
import Layout from './components/Layout';
import Dashboard from './pages/Dashboard';
import StreamManager from './pages/StreamManager';
import Settings from './pages/Settings';
import ZoneEditorPage from './pages/ZoneEditorPage';
import Logs from './pages/Logs';
import { Typography, Box } from '@mui/material';

// Placeholder pages
const PlaceholderPage = ({ title }: { title: string }) => (
  <Box p={3} textAlign="center">
    <Typography variant="h4" color="text.secondary">{title}</Typography>
    <Typography>This feature is under development.</Typography>
  </Box>
);

function App() {
  const [currentTab, setCurrentTab] = useState('dashboard');

  const renderContent = () => {
    switch (currentTab) {
      case 'dashboard': return <Dashboard />;
      case 'zones': return <ZoneEditorPage />;
      case 'streams': return <StreamManager />;
      case 'logs': return <Logs />;
      case 'settings': return <Settings />;
      default: return <Dashboard />;
    }
  };

  return (
    <ThemeProvider theme={theme}>
      <Layout currentTab={currentTab} onTabChange={setCurrentTab}>
        {renderContent()}
      </Layout>
    </ThemeProvider>
  );
}

export default App;