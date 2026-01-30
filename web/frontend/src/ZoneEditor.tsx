import React, { useEffect, useState, useRef } from 'react';
import { Box, Button, Card, CardContent, Typography, List, ListItem, ListItemText, IconButton } from '@mui/material';
import DeleteIcon from '@mui/icons-material/Delete';
import axios from 'axios';

interface Point {
    x: number;
    y: number;
}

interface Zone {
    id: number;
    name: string;
    points: [number, number][]; // Array of [x, y]
}

interface Config {
    rtsp_uri: string;
    model_path: string;
    database_path: string;
    alert_cooldown: number;
    mqtt: any;
    zones: Zone[];
}

// Canvas size matching model input (normalized to this view)
const CANVAS_WIDTH = 640;
const CANVAS_HEIGHT = 480; // 4:3 aspect ratio typically

export default function ZoneEditor() {
    const [config, setConfig] = useState<Config | null>(null);
    const [activeZone, setActiveZone] = useState<Zone | null>(null);
    const [points, setPoints] = useState<Point[]>([]);
    const canvasRef = useRef<HTMLCanvasElement>(null);

    // Load config
    useEffect(() => {
        axios.get('/api/config').then(res => {
            setConfig(res.data);
            if (res.data.zones.length > 0) {
                selectZone(res.data.zones[0]);
            }
        });
    }, []);

    // Draw canvas
    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;

        // Clear
        ctx.fillStyle = '#f0f0f0';
        ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);

        // Draw grid
        ctx.strokeStyle = '#ddd';
        ctx.beginPath();
        for (let x = 0; x <= CANVAS_WIDTH; x += 40) { ctx.moveTo(x, 0); ctx.lineTo(x, CANVAS_HEIGHT); }
        for (let y = 0; y <= CANVAS_HEIGHT; y += 40) { ctx.moveTo(0, y); ctx.lineTo(CANVAS_WIDTH, y); }
        ctx.stroke();

        // Draw existing zones (greyed out)
        config?.zones.forEach(zone => {
            if (zone.id === activeZone?.id) return; // Don't draw active zone here
            drawPolygon(ctx, zone.points.map(p => ({ x: p[0], y: p[1] })), '#999', false);
        });

        // Draw active zone being edited
        if (points.length > 0) {
            drawPolygon(ctx, points, 'red', true);
        }

    }, [config, points, activeZone]);

    const drawPolygon = (ctx: CanvasRenderingContext2D, pts: Point[], color: string, showPoints: boolean) => {
        if (pts.length === 0) return;
        
        ctx.beginPath();
        ctx.moveTo(pts[0].x, pts[0].y);
        for (let i = 1; i < pts.length; i++) {
            ctx.lineTo(pts[i].x, pts[i].y);
        }
        ctx.closePath();
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.stroke();
        ctx.fillStyle = color + '33'; // Semi-transparent
        ctx.fill();

        if (showPoints) {
            ctx.fillStyle = color;
            pts.forEach(p => {
                ctx.beginPath();
                ctx.arc(p.x, p.y, 4, 0, Math.PI * 2);
                ctx.fill();
            });
        }
    };

    const handleCanvasClick = (e: React.MouseEvent) => {
        if (!activeZone) return;
        const canvas = canvasRef.current;
        if (!canvas) return;

        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        setPoints([...points, { x, y }]);
    };

    const selectZone = (zone: Zone) => {
        setActiveZone(zone);
        setPoints(zone.points.map(p => ({ x: p[0], y: p[1] })));
    };

    const handleSave = async () => {
        if (!config || !activeZone) return;

        // Update active zone in config
        const updatedZones = config.zones.map(z => {
            if (z.id === activeZone.id) {
                return {
                    ...z,
                    points: points.map(p => [p.x, p.y] as [number, number])
                };
            }
            return z;
        });

        const newConfig = { ...config, zones: updatedZones };
        
        try {
            await axios.post('/api/config', newConfig);
            setConfig(newConfig);
            alert('Config Saved!');
        } catch (err) {
            console.error(err);
            alert('Failed to save.');
        }
    };

    const handleClear = () => {
        setPoints([]);
    };

    const handleAddZone = () => {
        if (!config) return;
        const newId = config.zones.length > 0 ? Math.max(...config.zones.map(z => z.id)) + 1 : 1;
        const newZone: Zone = {
            id: newId,
            name: `New Zone ${newId}`,
            points: []
        };
        const newConfig = { ...config, zones: [...config.zones, newZone] };
        setConfig(newConfig);
        selectZone(newZone);
    };

    return (
        <Box display="flex" gap={2} mt={2}>
            {/* Sidebar List */}
            <Card sx={{ width: 300 }}>
                <CardContent>
                    <Typography variant="h6">Zones</Typography>
                    <List>
                        {config?.zones.map(zone => (
                            <ListItem 
                                key={zone.id} 
                                button 
                                selected={activeZone?.id === zone.id}
                                onClick={() => selectZone(zone)}
                            >
                                <ListItemText primary={zone.name} secondary={`${zone.points.length} points`} />
                                <IconButton edge="end" aria-label="delete">
                                    <DeleteIcon />
                                </IconButton>
                            </ListItem>
                        ))}
                    </List>
                    <Button variant="contained" fullWidth onClick={handleAddZone}>
                        Add New Zone
                    </Button>
                </CardContent>
            </Card>

            {/* Editor Area */}
            <Card sx={{ flexGrow: 1 }}>
                <CardContent>
                    <Box display="flex" justifyContent="space-between" mb={2}>
                        <Typography variant="h6">Editor: {activeZone?.name}</Typography>
                        <Box>
                            <Button color="error" onClick={handleClear} sx={{ mr: 1 }}>Clear Points</Button>
                            <Button variant="contained" color="primary" onClick={handleSave}>Save Changes</Button>
                        </Box>
                    </Box>
                    
                    <Box display="flex" justifyContent="center" bgcolor="#333" p={2}>
                        <canvas 
                            ref={canvasRef}
                            width={CANVAS_WIDTH}
                            height={CANVAS_HEIGHT}
                            onClick={handleCanvasClick}
                            style={{ cursor: 'crosshair', backgroundColor: '#fff' }}
                        />
                    </Box>
                    <Typography variant="caption" display="block" align="center" sx={{ mt: 1 }}>
                        Click on the canvas to add points to the polygon.
                    </Typography>
                </CardContent>
            </Card>
        </Box>
    );
}
