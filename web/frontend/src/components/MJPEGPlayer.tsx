import { useState, useEffect, useRef } from 'react';
import { Box, Typography, CircularProgress } from '@mui/material';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';

interface MJPEGPlayerProps {
    url: string;
    aspectRatio?: string;
    label?: string;
}

export default function MJPEGPlayer({ url, aspectRatio = '16/9', label }: MJPEGPlayerProps) {
    const [status, setStatus] = useState<'loading' | 'live' | 'error'>('loading');
    const [retryCount, setRetryCount] = useState(0);
    const imgRef = useRef<HTMLImageElement>(null);
    const timeoutRef = useRef<NodeJS.Timeout | null>(null);

    // Effect to handle URL changes or retries
    useEffect(() => {
        setStatus('loading');
        if (imgRef.current) {
            // Append timestamp to prevent browser caching on retry
            const separator = url.includes('?') ? '&' : '?';
            imgRef.current.src = `${url}${separator}t=${Date.now()}`;
        }
    }, [url, retryCount]);

    const handleLoad = () => {
        setStatus('live');
        if (timeoutRef.current) clearTimeout(timeoutRef.current);
    };

    const handleError = () => {
        setStatus('error');
        // Exponential backoff for retry (max 10s)
        const delay = Math.min(1000 * Math.pow(1.5, retryCount), 10000);
        
        if (timeoutRef.current) clearTimeout(timeoutRef.current);
        timeoutRef.current = setTimeout(() => {
            setRetryCount(c => c + 1);
        }, delay);
    };

    return (
        <Box 
            sx={{ 
                width: '100%', 
                aspectRatio: aspectRatio, 
                bgcolor: '#000', 
                borderRadius: 1, 
                overflow: 'hidden', 
                position: 'relative',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center'
            }}
        >
            {/* The Image Stream */}
            <img 
                ref={imgRef}
                onLoad={handleLoad}
                onError={handleError}
                style={{ 
                    width: '100%', 
                    height: '100%', 
                    objectFit: 'contain',
                    opacity: status === 'live' ? 1 : 0 
                }}
                alt={label || "Live Stream"}
            />

            {/* Overlays for different states */}
            {status === 'loading' && (
                <Box position="absolute" display="flex" flexDirection="column" alignItems="center" color="white">
                    <CircularProgress color="inherit" size={40} thickness={4} />
                    <Typography variant="caption" sx={{ mt: 1 }}>Connecting...</Typography>
                </Box>
            )}

            {status === 'error' && (
                <Box position="absolute" display="flex" flexDirection="column" alignItems="center" color="grey.500">
                    <VideocamOffIcon sx={{ fontSize: 48, mb: 1 }} />
                    <Typography variant="body1">Stream Offline</Typography>
                    <Typography variant="caption">Retrying in a moment...</Typography>
                </Box>
            )}

            {/* Live Indicator Badge */}
            {status === 'live' && (
                <Box 
                    sx={{
                        position: 'absolute',
                        top: 10,
                        right: 10,
                        bgcolor: 'rgba(0,0,0,0.6)',
                        color: '#4caf50',
                        px: 1,
                        py: 0.5,
                        borderRadius: 1,
                        display: 'flex',
                        alignItems: 'center',
                        gap: 0.5,
                        fontSize: '0.75rem',
                        fontWeight: 'bold'
                    }}
                >
                    <Box 
                        sx={{ 
                            width: 8, 
                            height: 8, 
                            bgcolor: '#4caf50', 
                            borderRadius: '50%',
                            animation: 'pulse 2s infinite'
                        }} 
                    />
                    LIVE
                </Box>
            )}
            
            <style>
                {`
                @keyframes pulse {
                    0% { opacity: 1; }
                    50% { opacity: 0.5; }
                    100% { opacity: 1; }
                }
                `}
            </style>
        </Box>
    );
}
