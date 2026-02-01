import { useState } from 'react';
import { ThemeProvider } from '@mui/material/styles';
import { theme } from './theme';
import Layout from './components/Layout';
import Dashboard from './pages/Dashboard';
import ZoneEditor from './ZoneEditor'; // Using existing for now, will move later
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
      case 'zones': return <ZoneEditor />;
      case 'streams': return <PlaceholderPage title="Camera Management" />;
      case 'logs': return <PlaceholderPage title="Violation Logs" />;
      case 'settings': return <PlaceholderPage title="System Settings" />;
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