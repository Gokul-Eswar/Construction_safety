import { useEffect, useState } from 'react';
import { 
    Box, Typography, Button, Table, TableBody, TableCell, 
    TableContainer, TableHead, TableRow, Paper, IconButton, 
    Dialog, DialogTitle, DialogContent, DialogActions, TextField, Chip 
} from '@mui/material';
import DeleteIcon from '@mui/icons-material/Delete';
import EditIcon from '@mui/icons-material/Edit';
import AddIcon from '@mui/icons-material/Add';
import axios from 'axios';

interface Stream {
    id: string;
    name: string;
    rtsp_uri: string;
    zones?: any[];
}

export default function StreamManager() {
    const [streams, setStreams] = useState<Stream[]>([]);
    const [open, setOpen] = useState(false);
    const [editingId, setEditingId] = useState<string | null>(null);
    
    // Form State
    const [formId, setFormId] = useState('');
    const [formName, setFormName] = useState('');
    const [formUri, setFormUri] = useState('');

    useEffect(() => {
        fetchConfig();
    }, []);

    const fetchConfig = async () => {
        try {
            const res = await axios.get('/api/config');
            if (res.data.streams) {
                setStreams(res.data.streams);
            }
        } catch (err) {
            console.error(err);
        }
    };

    const handleSave = async () => {
        const newStream: Stream = {
            id: formId,
            name: formName,
            rtsp_uri: formUri,
            zones: [] // Default empty zones for new stream
        };

        let updatedStreams;
        if (editingId) {
            // Update existing
            updatedStreams = streams.map(s => {
                if (s.id === editingId) {
                    return { ...newStream, zones: s.zones }; // Keep existing zones
                }
                return s;
            });
        } else {
            // Add new
            if (streams.find(s => s.id === formId)) {
                alert('Stream ID must be unique');
                return;
            }
            updatedStreams = [...streams, newStream];
        }

        try {
            await axios.post('/api/config/streams', updatedStreams);
            setStreams(updatedStreams);
            setOpen(false);
            resetForm();
        } catch (err) {
            console.error(err);
            alert('Failed to save streams');
        }
    };

    const handleDelete = async (id: string) => {
        if (!confirm('Are you sure you want to delete this stream?')) return;
        
        const updatedStreams = streams.filter(s => s.id !== id);
        try {
            await axios.post('/api/config/streams', updatedStreams);
            setStreams(updatedStreams);
        } catch (err) {
            console.error(err);
        }
    };

    const handleEdit = (stream: Stream) => {
        setFormId(stream.id);
        setFormName(stream.name);
        setFormUri(stream.rtsp_uri);
        setEditingId(stream.id);
        setOpen(true);
    };

    const handleAdd = () => {
        resetForm();
        setOpen(true);
    };

    const resetForm = () => {
        setFormId('');
        setFormName('');
        setFormUri('');
        setEditingId(null);
    };

    return (
        <Box>
            <Box display="flex" justifyContent="space-between" mb={3}>
                <Typography variant="h5">Camera Management</Typography>
                <Button variant="contained" startIcon={<AddIcon />} onClick={handleAdd}>
                    Add Camera
                </Button>
            </Box>

            <TableContainer component={Paper}>
                <Table>
                    <TableHead>
                        <TableRow>
                            <TableCell>ID</TableCell>
                            <TableCell>Name</TableCell>
                            <TableCell>RTSP URI</TableCell>
                            <TableCell>Zones</TableCell>
                            <TableCell align="right">Actions</TableCell>
                        </TableRow>
                    </TableHead>
                    <TableBody>
                        {streams.map((stream) => (
                            <TableRow key={stream.id}>
                                <TableCell>{stream.id}</TableCell>
                                <TableCell>{stream.name}</TableCell>
                                <TableCell sx={{ fontFamily: 'monospace' }}>{stream.rtsp_uri}</TableCell>
                                <TableCell>
                                    <Chip label={stream.zones?.length || 0} size="small" />
                                </TableCell>
                                <TableCell align="right">
                                    <IconButton onClick={() => handleEdit(stream)}>
                                        <EditIcon />
                                    </IconButton>
                                    <IconButton color="error" onClick={() => handleDelete(stream.id)}>
                                        <DeleteIcon />
                                    </IconButton>
                                </TableCell>
                            </TableRow>
                        ))}
                        {streams.length === 0 && (
                            <TableRow>
                                <TableCell colSpan={5} align="center">
                                    No cameras configured. Add one to get started.
                                </TableCell>
                            </TableRow>
                        )}
                    </TableBody>
                </Table>
            </TableContainer>

            <Dialog open={open} onClose={() => setOpen(false)}>
                <DialogTitle>{editingId ? 'Edit Camera' : 'Add New Camera'}</DialogTitle>
                <DialogContent>
                    <TextField
                        autoFocus
                        margin="dense"
                        label="Stream ID (Unique)"
                        fullWidth
                        value={formId}
                        onChange={(e) => setFormId(e.target.value)}
                        disabled={!!editingId} // Cannot change ID once created
                    />
                    <TextField
                        margin="dense"
                        label="Friendly Name"
                        fullWidth
                        value={formName}
                        onChange={(e) => setFormName(e.target.value)}
                    />
                    <TextField
                        margin="dense"
                        label="RTSP URI"
                        fullWidth
                        value={formUri}
                        onChange={(e) => setFormUri(e.target.value)}
                        helperText="e.g. rtsp://192.168.1.100:554/live"
                    />
                </DialogContent>
                <DialogActions>
                    <Button onClick={() => setOpen(false)}>Cancel</Button>
                    <Button onClick={handleSave} variant="contained">Save</Button>
                </DialogActions>
            </Dialog>
        </Box>
    );
}
