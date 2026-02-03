import { createTheme } from '@mui/material/styles';

export const theme = createTheme({
  palette: {
    mode: 'dark',
    primary: {
      main: '#ffcc00', // High-visibility Safety Yellow
      contrastText: '#000000',
    },
    secondary: {
      main: '#ff3d00', // Alert Orange/Red
    },
    background: {
      default: '#0a0a0a', // Deep Black
      paper: '#141414',   // Dark Gray Card
    },
    success: {
        main: '#00e676',
    },
    error: {
        main: '#ff1744',
    },
    text: {
        primary: '#ffffff',
        secondary: '#b0bec5',
    }
  },
  typography: {
    fontFamily: '"Roboto Mono", "Roboto", "Helvetica", "Arial", sans-serif',
    h4: {
        fontWeight: 600,
        letterSpacing: '-0.5px',
    },
    h6: {
        fontWeight: 500,
    }
  },
  components: {
    MuiCard: {
        styleOverrides: {
            root: {
                borderRadius: 8,
                border: '1px solid #333',
                backgroundImage: 'none', // Remove elevation gradient
            }
        }
    },
    MuiChip: {
        styleOverrides: {
            root: {
                borderRadius: 4,
                fontWeight: 'bold',
            }
        }
    }
  }
});