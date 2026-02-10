import { useState, useEffect } from 'react';
import { 
    Box, Typography, Paper, Table, TableBody, TableCell, 
    TableContainer, TableHead, TableRow, TablePagination,
    Chip, CircularProgress, IconButton
} from '@mui/material';
import WarningIcon from '@mui/icons-material/Warning';
import CloudDoneIcon from '@mui/icons-material/CloudDone';
import CloudOffIcon from '@mui/icons-material/CloudOff';
import RefreshIcon from '@mui/icons-material/Refresh';
import axios from 'axios';

interface Violation {
    id: number;
    timestamp: string;
    zone_id: number;
    zone_name: string;
    confidence: number;
    object_id: number;
    uploaded: number;
}

export default function Logs() {
    const [page, setPage] = useState(0);
    const [rowsPerPage, setRowsPerPage] = useState(10);
    const [rows, setRows] = useState<Violation[]>([]);
    const [total, setTotal] = useState(0);
    const [loading, setLoading] = useState(true);

    const fetchLogs = async () => {
        setLoading(true);
        try {
            const offset = page * rowsPerPage;
            const res = await axios.get(`/api/violations?limit=${rowsPerPage}&offset=${offset}`);
            setRows(res.data.data);
            setTotal(res.data.pagination.total);
        } catch (err) {
            console.error("Failed to fetch logs:", err);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        fetchLogs();
    }, [page, rowsPerPage]);

    const handleChangePage = (event: unknown, newPage: number) => {
        setPage(newPage);
    };

    const handleChangeRowsPerPage = (event: React.ChangeEvent<HTMLInputElement>) => {
        setRowsPerPage(parseInt(event.target.value, 10));
        setPage(0);
    };

    return (
        <Box>
            <Box display="flex" justifyContent="space-between" alignItems="center" mb={3}>
                <Typography variant="h4">Violation Logs</Typography>
                <IconButton onClick={fetchLogs} color="primary">
                    <RefreshIcon />
                </IconButton>
            </Box>

            <Paper sx={{ width: '100%', overflow: 'hidden' }}>
                <TableContainer>
                    <Table stickyHeader>
                        <TableHead>
                            <TableRow>
                                <TableCell>ID</TableCell>
                                <TableCell>Timestamp</TableCell>
                                <TableCell>Zone</TableCell>
                                <TableCell>Confidence</TableCell>
                                <TableCell>Track ID</TableCell>
                                <TableCell align="center">Cloud Status</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {loading ? (
                                <TableRow>
                                    <TableCell colSpan={6} align="center">
                                        <CircularProgress size={24} />
                                    </TableCell>
                                </TableRow>
                            ) : rows.length === 0 ? (
                                <TableRow>
                                    <TableCell colSpan={6} align="center">
                                        No violations recorded.
                                    </TableCell>
                                </TableRow>
                            ) : (
                                rows.map((row) => (
                                    <TableRow key={row.id} hover>
                                        <TableCell>{row.id}</TableCell>
                                        <TableCell>{row.timestamp}</TableCell>
                                        <TableCell>
                                            <Chip 
                                                icon={<WarningIcon />} 
                                                label={row.zone_name} 
                                                color="warning" 
                                                size="small" 
                                                variant="outlined" 
                                            />
                                        </TableCell>
                                        <TableCell>{(row.confidence * 100).toFixed(1)}%</TableCell>
                                        <TableCell>{row.object_id > -1 ? row.object_id : 'N/A'}</TableCell>
                                        <TableCell align="center">
                                            {row.uploaded ? (
                                                <CloudDoneIcon color="success" fontSize="small" titleAccess="Synced to Cloud" />
                                            ) : (
                                                <CloudOffIcon color="disabled" fontSize="small" titleAccess="Pending Upload" />
                                            )}
                                        </TableCell>
                                    </TableRow>
                                ))
                            )}
                        </TableBody>
                    </Table>
                </TableContainer>
                <TablePagination
                    rowsPerPageOptions={[10, 25, 100]}
                    component="div"
                    count={total}
                    rowsPerPage={rowsPerPage}
                    page={page}
                    onPageChange={handleChangePage}
                    onRowsPerPageChange={handleChangeRowsPerPage}
                />
            </Paper>
        </Box>
    );
}
