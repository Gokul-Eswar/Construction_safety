const mqtt = require('mqtt');

const host = process.env.MQTT_HOST || 'localhost';
const port = Number.parseInt(process.env.MQTT_PORT || '1883', 10);
const durationSec = Number.parseInt(process.env.DURATION_SEC || '60', 10);

const topics = [
  'safety/v1/telemetry',
  'safety/v1/heartbeat',
  'safety/v1/violations',
  'safety/v1/cloud_sync'
];

const state = {
  started_at: new Date().toISOString(),
  host,
  port,
  duration_sec: durationSec,
  counts: {
    telemetry: 0,
    heartbeat: 0,
    violations: 0,
    cloud_sync: 0,
    parse_errors: 0
  },
  telemetry_windows: [],
  streams: {},
  latency_keys: {},
  gpu: [],
  heartbeat_statuses: {},
  first_event_ts: null,
  last_event_ts: null
};

function addStreamSample(streamId, sample) {
  if (!state.streams[streamId]) {
    state.streams[streamId] = {
      fps: [],
      reconnect_count: [],
      error_count: [],
      frame_count: [],
      low_vram_events: []
    };
  }
  const s = state.streams[streamId];
  if (Number.isFinite(sample.fps)) s.fps.push(sample.fps);
  if (Number.isFinite(sample.reconnect_count)) s.reconnect_count.push(sample.reconnect_count);
  if (Number.isFinite(sample.error_count)) s.error_count.push(sample.error_count);
  if (Number.isFinite(sample.frame_count)) s.frame_count.push(sample.frame_count);
  if (Number.isFinite(sample.low_vram_events)) s.low_vram_events.push(sample.low_vram_events);
}

function addLatencySample(key, val) {
  if (!state.latency_keys[key]) {
    state.latency_keys[key] = {
      avg: [],
      min: [],
      max: [],
      p99: []
    };
  }
  const dst = state.latency_keys[key];
  if (Number.isFinite(val.avg)) dst.avg.push(val.avg);
  if (Number.isFinite(val.min)) dst.min.push(val.min);
  if (Number.isFinite(val.max)) dst.max.push(val.max);
  if (Number.isFinite(val.p99)) dst.p99.push(val.p99);
}

function stats(values) {
  if (!values || values.length === 0) return null;
  const sorted = [...values].sort((a, b) => a - b);
  const sum = values.reduce((a, b) => a + b, 0);
  const idx95 = Math.min(sorted.length - 1, Math.ceil(0.95 * sorted.length) - 1);
  const idx99 = Math.min(sorted.length - 1, Math.ceil(0.99 * sorted.length) - 1);
  return {
    n: values.length,
    avg: Number((sum / values.length).toFixed(3)),
    min: Number(sorted[0].toFixed(3)),
    p95: Number(sorted[idx95].toFixed(3)),
    p99: Number(sorted[idx99].toFixed(3)),
    max: Number(sorted[sorted.length - 1].toFixed(3))
  };
}

function recordEventTimestamp() {
  const now = new Date().toISOString();
  if (!state.first_event_ts) state.first_event_ts = now;
  state.last_event_ts = now;
}

function summarize() {
  const summary = {
    generated_at: new Date().toISOString(),
    input: {
      host: state.host,
      port: state.port,
      duration_sec: state.duration_sec,
      topics
    },
    counts: state.counts,
    stream_fps: {},
    latency_ms: {},
    gpu: {
      utilization_percent: null,
      temperature_c: null,
      memory_used_mb: null,
      memory_total_mb: null
    },
    heartbeat_statuses: state.heartbeat_statuses,
    event_window: {
      first_event_ts: state.first_event_ts,
      last_event_ts: state.last_event_ts
    },
    notes: []
  };

  for (const [streamId, s] of Object.entries(state.streams)) {
    summary.stream_fps[streamId] = {
      fps: stats(s.fps),
      reconnect_count: stats(s.reconnect_count),
      error_count: stats(s.error_count),
      frame_count: stats(s.frame_count),
      low_vram_events: stats(s.low_vram_events)
    };
  }

  for (const [key, v] of Object.entries(state.latency_keys)) {
    summary.latency_ms[key] = {
      avg_reported: stats(v.avg),
      min_reported: stats(v.min),
      max_reported: stats(v.max),
      p99_reported: stats(v.p99)
    };
  }

  if (state.gpu.length > 0) {
    summary.gpu.utilization_percent = stats(state.gpu.map((g) => g.utilization));
    summary.gpu.temperature_c = stats(state.gpu.map((g) => g.temperature));
    summary.gpu.memory_used_mb = stats(state.gpu.map((g) => g.memory_used_mb));
    summary.gpu.memory_total_mb = stats(state.gpu.map((g) => g.memory_total_mb));
  } else {
    summary.notes.push('No GPU telemetry received in capture window.');
  }

  if (state.counts.telemetry === 0) {
    summary.notes.push('No telemetry messages were received. Ensure MQTT broker and engine are both running and connected.');
  }

  return summary;
}

const client = mqtt.connect(`mqtt://${host}:${port}`);

client.on('connect', () => {
  client.subscribe(topics, (err) => {
    if (err) {
      console.error('[MQTT] Subscribe failed:', err.message);
      process.exit(1);
    }
    console.error(`[MQTT] Connected. Capturing for ${durationSec}s from ${host}:${port}`);
  });
});

client.on('message', (topic, msg) => {
  recordEventTimestamp();

  let payload;
  try {
    payload = JSON.parse(msg.toString());
  } catch (e) {
    state.counts.parse_errors += 1;
    return;
  }

  if (topic === 'safety/v1/telemetry') {
    state.counts.telemetry += 1;

    if (payload.streams && typeof payload.streams === 'object') {
      for (const [streamId, s] of Object.entries(payload.streams)) {
        addStreamSample(streamId, s || {});
      }
    }

    if (payload.latency && typeof payload.latency === 'object') {
      for (const [key, val] of Object.entries(payload.latency)) {
        addLatencySample(key, val || {});
      }
    }

    if (payload.gpu && typeof payload.gpu === 'object') {
      const g = payload.gpu;
      state.gpu.push({
        utilization: Number(g.utilization || 0),
        temperature: Number(g.temperature || 0),
        memory_used_mb: Number(g.memory_used_mb || 0),
        memory_total_mb: Number(g.memory_total_mb || 0)
      });
    }
  } else if (topic === 'safety/v1/heartbeat') {
    state.counts.heartbeat += 1;
    const key = String(payload.status || 'unknown');
    state.heartbeat_statuses[key] = (state.heartbeat_statuses[key] || 0) + 1;
  } else if (topic === 'safety/v1/violations') {
    state.counts.violations += 1;
  } else if (topic === 'safety/v1/cloud_sync') {
    state.counts.cloud_sync += 1;
  }
});

client.on('error', (err) => {
  console.error('[MQTT] Error:', err.message);
});

setTimeout(() => {
  const output = summarize();
  console.log(JSON.stringify(output, null, 2));
  client.end(true, () => process.exit(0));
}, Math.max(5, durationSec) * 1000);
