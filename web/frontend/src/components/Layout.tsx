import React, { useState } from 'react';
import { 
  AppBar, Box, CssBaseline, Drawer, IconButton, List, ListItem, 
  ListItemButton, ListItemIcon, ListItemText, Toolbar, Typography, 
  Divider 
} from '@mui/material';
import MenuIcon from '@mui/icons-material/Menu';
import DashboardIcon from '@mui/icons-material/Dashboard';
import VideocamIcon from '@mui/icons-material/Videocam';
import EditLocationIcon from '@mui/icons-material/EditLocation';
import SettingsIcon from '@mui/icons-material/Settings';
import HistoryIcon from '@mui/icons-material/History';
import WarningIcon from '@mui/icons-material/Warning';
import StraightenIcon from '@mui/icons-material/Straighten';

const drawerWidth = 240;

interface LayoutProps {
  children: React.ReactNode;
  currentTab: string;
  onTabChange: (tab: string) => void;
}

export default function Layout({ children, currentTab, onTabChange }: LayoutProps) {
  const [mobileOpen, setMobileOpen] = useState(false);

  const handleDrawerToggle = () => {
    setMobileOpen(!mobileOpen);
  };

  const menuItems = [
    { id: 'dashboard', text: 'Dashboard', icon: <DashboardIcon /> },
    { id: 'streams', text: 'Cameras', icon: <VideocamIcon /> },
    { id: 'zones', text: 'Safety Zones', icon: <EditLocationIcon /> },
    { id: 'calibration', text: 'Calibration', icon: <StraightenIcon /> },
    { id: 'logs', text: 'Violation Logs', icon: <HistoryIcon /> },
    { id: 'settings', text: 'System Settings', icon: <SettingsIcon /> },
  ];

  const drawer = (
    <div>
      <Toolbar sx={{ justifyContent: 'center' }}>
         <Box display="flex" alignItems="center" color="primary.main" gap={1}>
            <WarningIcon />
            <Typography variant="h6" noWrap component="div" sx={{ fontWeight: 'bold' }}>
              SENTINEL
            </Typography>
         </Box>
      </Toolbar>
      <Divider />
      <List>
        {menuItems.map((item) => (
          <ListItem key={item.id} disablePadding>
            <ListItemButton 
              selected={currentTab === item.id}
              onClick={() => {
                onTabChange(item.id);
                setMobileOpen(false);
              }}
              sx={{
                '&.Mui-selected': {
                  backgroundColor: 'rgba(255, 215, 0, 0.12)', // Gold tint
                  borderLeft: '4px solid #FFD700',
                  '&:hover': { backgroundColor: 'rgba(255, 215, 0, 0.2)' },
                },
                pl: currentTab === item.id ? 1.5 : 2, // Adjust padding for border
              }}
            >
              <ListItemIcon sx={{ color: currentTab === item.id ? 'primary.main' : 'inherit' }}>
                {item.icon}
              </ListItemIcon>
              <ListItemText primary={item.text} />
            </ListItemButton>
          </ListItem>
        ))}
      </List>
      <Box sx={{ flexGrow: 1 }} />
      <Divider />
      <Box p={2}>
        <Typography variant="caption" color="text.secondary" display="block" align="center">
          v1.2.0 • Online
        </Typography>
      </Box>
    </div>
  );

  return (
    <Box sx={{ display: 'flex' }}>
      <CssBaseline />
      <AppBar
        position="fixed"
        sx={{
          width: { sm: `calc(100% - ${drawerWidth}px)` },
          ml: { sm: `${drawerWidth}px` },
        }}
      >
        <Toolbar>
          <IconButton
            color="inherit"
            aria-label="open drawer"
            edge="start"
            onClick={handleDrawerToggle}
            sx={{ mr: 2, display: { sm: 'none' } }}
          >
            <MenuIcon />
          </IconButton>
          <Typography variant="h6" noWrap component="div" color="primary">
            {menuItems.find(i => i.id === currentTab)?.text || 'Dashboard'}
          </Typography>
          <Box sx={{ flexGrow: 1 }} />
          {/* Add Status Indicators or User Profile here if needed */}
        </Toolbar>
      </AppBar>
      <Box
        component="nav"
        sx={{ width: { sm: drawerWidth }, flexShrink: { sm: 0 } }}
        aria-label="mailbox folders"
      >
        <Drawer
          variant="temporary"
          open={mobileOpen}
          onClose={handleDrawerToggle}
          ModalProps={{
            keepMounted: true, // Better open performance on mobile.
          }}
          sx={{
            display: { xs: 'block', sm: 'none' },
            '& .MuiDrawer-paper': { boxSizing: 'border-box', width: drawerWidth },
          }}
        >
          {drawer}
        </Drawer>
        <Drawer
          variant="permanent"
          sx={{
            display: { xs: 'none', sm: 'block' },
            '& .MuiDrawer-paper': { boxSizing: 'border-box', width: drawerWidth },
          }}
          open
        >
          {drawer}
        </Drawer>
      </Box>
      <Box
        component="main"
        sx={{ flexGrow: 1, p: 3, width: { sm: `calc(100% - ${drawerWidth}px)` } }}
      >
        <Toolbar />
        {children}
      </Box>
    </Box>
  );
}
