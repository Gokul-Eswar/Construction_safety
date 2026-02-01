import React, { useEffect, useState, useRef } from 'react';
import { 
    Box, Button, Card, CardContent, Typography, List, ListItem, 
    ListItemText, IconButton, Select, MenuItem, FormControl, InputLabel 
} from '@mui/material';
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

interface Stream {
    id: string;
    name: string;
    zones: Zone[];
}

// Canvas size matching model input (normalized to this view)
const CANVAS_WIDTH = 640;
const CANVAS_HEIGHT = 480;

export default function ZoneEditorPage() {
    const [streams, setStreams] = useState<Stream[]>([]);
    const [selectedStreamId, setSelectedStreamId] = useState<string>('');
    const [activeZone, setActiveZone] = useState<Zone | null>(null);
    const [points, setPoints] = useState<Point[]>([]);
    const canvasRef = useRef<HTMLCanvasElement>(null);

    // Load config
    useEffect(() => {
        axios.get('/api/config').then(res => {
            if (res.data.streams && res.data.streams.length > 0) {
                setStreams(res.data.streams);
                setSelectedStreamId(res.data.streams[0].id);
                if (res.data.streams[0].zones.length > 0) {
                    selectZone(res.data.streams[0].zones[0]);
                } else {
                    setActiveZone(null);
                    setPoints([]);
                }
            }
        });
    }, []);

    // Get current stream object
    const currentStream = streams.find(s => s.id === selectedStreamId);

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

        if (currentStream) {
            // Draw existing zones (greyed out)
            currentStream.zones.forEach(zone => {
                if (zone.id === activeZone?.id) return; // Don't draw active zone here
                drawPolygon(ctx, zone.points.map(p => ({ x: p[0], y: p[1] })), '#999', false);
            });
        }

        // Draw active zone being edited
        if (points.length > 0) {
            drawPolygon(ctx, points, 'red', true);
        }

    }, [currentStream, points, activeZone]);

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
        if (!currentStream || !activeZone) return;

        // Update zones for current stream
        const updatedZones = currentStream.zones.map(z => {
            if (z.id === activeZone.id) {
                return {
                    ...z,
                    points: points.map(p => [p.x, p.y] as [number, number])
                };
            }
            return z;
        });

        // Update streams array
        const updatedStreams = streams.map(s => {
            if (s.id === currentStream.id) {
                return { ...s, zones: updatedZones };
            }
            return s;
        });

        try {
            await axios.post('/api/config/streams', updatedStreams);
            setStreams(updatedStreams);
            alert('Zones Saved!');
        } catch (err) {
            console.error(err);
            alert('Failed to save.');
        }
    };

    const handleClear = () => {
        setPoints([]);
    };

    const handleAddZone = () => {
        if (!currentStream) return;
        // Determine new ID (max of all zones across all streams to be safe, or just per stream)
        // Let's keep it simple: max ID in current stream + 1
        const maxId = currentStream.zones.length > 0 ? Math.max(...currentStream.zones.map(z => z.id)) : 0;
        const newId = maxId + 1;
        
        const newZone: Zone = {
            id: newId,
            name: `Zone ${newId}`,
            points: []
        };

        const updatedStreams = streams.map(s => {
            if (s.id === currentStream.id) {
                return { ...s, zones: [...s.zones, newZone] };
            }
            return s;
        });

        setStreams(updatedStreams);
        // We select the new zone immediately, but we haven't saved it to backend yet.
        // It will be saved when user clicks "Save Changes" on the canvas.
        // BUT: Switching streams would lose it. Better to save immediately or block switch.
        // For prototype simplicity, we just update local state.
        
        // Wait, we need to update 'currentStream' reference or force re-render logic.
        // Since 'streams' state updated, 'currentStream' derived var will update on next render.
        // But we need to select it.
        setTimeout(() => selectZone(newZone), 0);
    };

    const handleDeleteZone = async (zoneId: number) => {
        if (!currentStream) return;
        if (!confirm('Delete zone?')) return;

        const updatedZones = currentStream.zones.filter(z => z.id !== zoneId);
        const updatedStreams = streams.map(s => {
            if (s.id === currentStream.id) {
                return { ...s, zones: updatedZones };
            }
            return s;
        });

        try {
            await axios.post('/api/config/streams', updatedStreams);
            setStreams(updatedStreams);
            setActiveZone(null);
            setPoints([]);
        } catch (err) {
            console.error(err);
        }
    };

    return (
        <Box display="flex" gap={2} flexDirection={{ xs: 'column', md: 'row' }}>
            {/* Sidebar List */}
            <Card sx={{ width: { xs: '100%', md: 300 } }}>
                <CardContent>
                    <FormControl fullWidth sx={{ mb: 2 }}>
                        <InputLabel>Select Camera</InputLabel>
                        <Select
                            value={selectedStreamId}
                            label="Select Camera"
                            onChange={(e) => setSelectedStreamId(e.target.value)}
                        >
                            {streams.map(s => (
                                <MenuItem key={s.id} value={s.id}>{s.name} ({s.id})</MenuItem>
                            ))}
                        </Select>
                    </FormControl>

                    <Typography variant="h6" gutterBottom>Zones</Typography>
                    <List>
                        {currentStream?.zones.map(zone => (
                            <ListItem 
                                key={zone.id} 
                                button 
                                selected={activeZone?.id === zone.id}
                                onClick={() => selectZone(zone)}
                                secondaryAction={
                                    <IconButton edge="end" aria-label="delete" onClick={() => handleDeleteZone(zone.id)}>
                                        <DeleteIcon />
                                    </IconButton>
                                }
                            >
                                <ListItemText primary={zone.name} secondary={`${zone.points.length} points`} />
                            </ListItem>
                        ))}
                    </List>
                    <Button variant="contained" fullWidth onClick={handleAddZone} disabled={!currentStream}>
                        Add New Zone
                    </Button>
                </CardContent>
            </Card>

            {/* Editor Area */}
            <Card sx={{ flexGrow: 1 }}>
                <CardContent>
                    <Box display="flex" justifyContent="space-between" mb={2} flexWrap="wrap" gap={1}>
                        <Typography variant="h6">Editor: {activeZone?.name || 'No Zone Selected'}</Typography>
                        <Box>
                            <Button color="error" onClick={handleClear} sx={{ mr: 1 }} disabled={!activeZone}>Clear Points</Button>
                            <Button variant="contained" color="primary" onClick={handleSave} disabled={!activeZone}>Save Changes</Button>
                        </Box>
                    </Box>
                    
                    <Box display="flex" justifyContent="center" bgcolor="#333" p={2} overflow="auto">
                        <canvas 
                            ref={canvasRef}
                            width={CANVAS_WIDTH}
                            height={CANVAS_HEIGHT}
                            onClick={handleCanvasClick}
                            style={{ cursor: activeZone ? 'crosshair' : 'default', backgroundColor: '#fff', maxWidth: '100%' }}
                        />
                    </Box>
                    <Typography variant="caption" display="block" align="center" sx={{ mt: 1 }}>
                        Select a camera and zone, then click to define the safety boundary.
                    </Typography>
                </CardContent>
            </Card>
        </Box>
    );
}
