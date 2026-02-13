import React, { useEffect, useState, useRef } from 'react';
import { 
    Box, Button, Card, CardContent, Typography, TextField, 
    Table, TableBody, TableCell, TableHead, TableRow, 
    Select, MenuItem, FormControl, InputLabel, Paper
} from '@mui/material';
import axios from 'axios';

interface Point2D {
    x: number;
    y: number;
}

interface CalibrationPoint {
    image: Point2D;
    world: Point2D;
}

interface Stream {
    id: string;
    name: string;
    calibration?: CalibrationPoint[];
}

const CANVAS_WIDTH = 640;
const CANVAS_HEIGHT = 480;

export default function CalibrationPage() {
    const [streams, setStreams] = useState<Stream[]>([]);
    const [selectedStreamId, setSelectedStreamId] = useState<string>('');
    const [points, setPoints] = useState<CalibrationPoint[]>([]);
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        axios.get('/api/config').then(res => {
            if (res.data.streams && res.data.streams.length > 0) {
                setStreams(res.data.streams);
                const first = res.data.streams[0];
                setSelectedStreamId(first.id);
                setPoints(first.calibration || []);
            }
        });
    }, []);

    const handleStreamChange = (id: string) => {
        setSelectedStreamId(id);
        const stream = streams.find(s => s.id === id);
        setPoints(stream?.calibration || []);
    };

    // Draw canvas
    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;

        // Background
        ctx.fillStyle = '#1a1a1a';
        ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);

        // Grid
        ctx.strokeStyle = '#333';
        ctx.beginPath();
        for (let x = 0; x <= CANVAS_WIDTH; x += 40) { ctx.moveTo(x, 0); ctx.lineTo(x, CANVAS_HEIGHT); }
        for (let y = 0; y <= CANVAS_HEIGHT; y += 40) { ctx.moveTo(0, y); ctx.lineTo(CANVAS_WIDTH, y); }
        ctx.stroke();

        // Draw points
        points.forEach((p, index) => {
            // Point
            ctx.fillStyle = '#ffD700';
            ctx.beginPath();
            ctx.arc(p.image.x, p.image.y, 6, 0, Math.PI * 2);
            ctx.fill();
            
            // Label
            ctx.fillStyle = 'white';
            ctx.font = 'bold 12px Arial';
            ctx.fillText(`P${index + 1}`, p.image.x + 8, p.image.y - 8);
            
            // Text info
            ctx.font = '10px Arial';
            ctx.fillText(`W: ${p.world.x}, ${p.world.y}`, p.image.x + 8, p.image.y + 4);
        });

        // If we have 4+ points, draw a connecting polygon to show the ground plane
        if (points.length >= 3) {
            ctx.beginPath();
            ctx.moveTo(points[0].image.x, points[0].image.y);
            for (let i = 1; i < points.length; i++) {
                ctx.lineTo(points[i].image.x, points[i].image.y);
            }
            ctx.closePath();
            ctx.strokeStyle = '#ffD700';
            ctx.lineWidth = 1;
            ctx.stroke();
            ctx.fillStyle = 'rgba(255, 215, 0, 0.1)';
            ctx.fill();
        }

    }, [points]);

    const handleCanvasClick = (e: React.MouseEvent) => {
        const canvas = canvasRef.current;
        if (!canvas) return;

        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        // If we click near an existing point, maybe select it? 
        // For now, just add a new one until 8 points.
        if (points.length < 8) {
            setPoints([...points, { 
                image: { x: Math.round(x), y: Math.round(y) }, 
                world: { x: 0, y: 0 } 
            }]);
        }
    };

    const updateWorldCoord = (index: number, axis: 'x' | 'y', value: string) => {
        const num = parseFloat(value) || 0;
        const newPoints = [...points];
        newPoints[index].world[axis] = num;
        setPoints(newPoints);
    };

    const removePoint = (index: number) => {
        setPoints(points.filter((_, i) => i !== index));
    };

    const handleSave = async () => {
        const updatedStreams = streams.map(s => {
            if (s.id === selectedStreamId) {
                return { ...s, calibration: points };
            }
            return s;
        });

        try {
            await axios.post('/api/config/streams', updatedStreams);
            setStreams(updatedStreams);
            alert('Calibration Saved! The engine will now use ground-plane mapping for this camera.');
        } catch (err) {
            console.error(err);
            alert('Failed to save calibration.');
        }
    };

    return (
        <Box display="flex" flexDirection="column" gap={3}>
            <Card>
                <CardContent>
                    <Typography variant="h5" gutterBottom color="primary" sx={{ fontWeight: 'bold' }}>
                        Perspective Calibration (Ground Plane)
                    </Typography>
                    <Typography variant="body2" color="text.secondary" paragraph>
                        Map image coordinates to real-world ground coordinates to eliminate perspective errors. 
                        A person is only alerted if their <b>feet</b> touch the ground inside a zone.
                    </Typography>
                    
                    <Box display="flex" gap={2} alignItems="center" mb={3}>
                        <FormControl sx={{ minWidth: 300 }}>
                            <InputLabel>Select Camera</InputLabel>
                            <Select
                                value={selectedStreamId}
                                label="Select Camera"
                                onChange={(e) => handleStreamChange(e.target.value)}
                            >
                                {streams.map(s => (
                                    <MenuItem key={s.id} value={s.id}>{s.name} ({s.id})</MenuItem>
                                ))}
                            </Select>
                        </FormControl>
                        <Button variant="contained" color="primary" onClick={handleSave} disabled={points.length < 4}>
                            Apply Calibration
                        </Button>
                        <Button variant="outlined" color="error" onClick={() => setPoints([])}>
                            Reset
                        </Button>
                    </Box>

                    <Box display="flex" gap={2} flexDirection={{ xs: 'column', lg: 'row' }}>
                        {/* Canvas Area */}
                        <Paper elevation={4} sx={{ p: 1, bgcolor: '#000', display: 'flex', justifyContent: 'center' }}>
                            <canvas 
                                ref={canvasRef}
                                width={CANVAS_WIDTH}
                                height={CANVAS_HEIGHT}
                                onClick={handleCanvasClick}
                                style={{ cursor: 'crosshair', maxWidth: '100%' }}
                            />
                        </Paper>

                        {/* Point Table */}
                        <Box sx={{ flexGrow: 1 }}>
                            <Typography variant="subtitle1" gutterBottom>Calibration Points (Min 4)</Typography>
                            <Table size="small">
                                <TableHead>
                                    <TableRow>
                                        <TableCell>Point</TableCell>
                                        <TableCell>Image (X, Y)</TableCell>
                                        <TableCell>World X (m)</TableCell>
                                        <TableCell>World Y (m)</TableCell>
                                        <TableCell>Action</TableCell>
                                    </TableRow>
                                </TableHead>
                                <TableBody>
                                    {points.map((p, i) => (
                                        <TableRow key={i}>
                                            <TableCell>P{i+1}</TableCell>
                                            <TableCell>{p.image.x}, {p.image.y}</TableCell>
                                            <TableCell>
                                                <TextField 
                                                    size="small" 
                                                    type="number"
                                                    value={p.world.x} 
                                                    onChange={(e) => updateWorldCoord(i, 'x', e.target.value)}
                                                    sx={{ width: 80 }}
                                                />
                                            </TableCell>
                                            <TableCell>
                                                <TextField 
                                                    size="small" 
                                                    type="number"
                                                    value={p.world.y} 
                                                    onChange={(e) => updateWorldCoord(i, 'y', e.target.value)}
                                                    sx={{ width: 80 }}
                                                />
                                            </TableCell>
                                            <TableCell>
                                                <Button color="error" size="small" onClick={() => removePoint(i)}>Remove</Button>
                                            </TableCell>
                                        </TableRow>
                                    ))}
                                    {points.length === 0 && (
                                        <TableRow>
                                            <TableCell colSpan={5} align="center">
                                                Click on the image to add calibration points.
                                            </TableCell>
                                        </TableRow>
                                    )}
                                </TableBody>
                            </Table>
                            <Box mt={2} p={2} bgcolor="rgba(0,0,0,0.05)" borderRadius={1}>
                                <Typography variant="caption" color="text.secondary">
                                    <b>Instructions:</b><br/>
                                    1. Click 4 points on the ground in the video feed (e.g., corners of a room or square tiles).<br/>
                                    2. Enter the corresponding real-world coordinates for each point in meters.<br/>
                                    3. The system will calculate the homography matrix to "flatten" the perspective.
                                </Typography>
                            </Box>
                        </Box>
                    </Box>
                </CardContent>
            </Card>
        </Box>
    );
}
