#!/usr/bin/env python3
"""Generate report graphs from YOLO training outputs and project snapshots.

This script intentionally avoids third-party plotting libraries so it can run in
minimal environments. It writes SVG files for:
- training metrics curves from Ultralytics results.csv
- confusion matrix heatmaps from JSON or CSV matrix inputs
- runtime telemetry summary graphs from metrics_runtime_snapshot.json
- database summary graphs from metrics_db_snapshot.json
"""

from __future__ import annotations

import argparse
import csv
import json
import math
from collections import OrderedDict
from pathlib import Path
from typing import Any, Iterable
from xml.sax.saxutils import escape

CANVAS_WIDTH = 1200
CANVAS_HEIGHT = 760
MARGIN_LEFT = 80
MARGIN_RIGHT = 50
MARGIN_TOP = 80
MARGIN_BOTTOM = 90
PLOT_WIDTH = CANVAS_WIDTH - MARGIN_LEFT - MARGIN_RIGHT
PLOT_HEIGHT = CANVAS_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM
PALETTE = [
    "#1f77b4",
    "#d62728",
    "#2ca02c",
    "#9467bd",
    "#ff7f0e",
    "#17becf",
    "#8c564b",
    "#e377c2",
]


def read_json(path: Path) -> Any:
    for encoding in ("utf-8-sig", "utf-8", "utf-16"):
        try:
            with path.open("r", encoding=encoding) as handle:
                return json.load(handle)
        except (UnicodeError, json.JSONDecodeError):
            continue
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        return json.load(handle)


def read_csv_rows(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def safe_float(value: Any) -> float | None:
    if value is None:
        return None
    if isinstance(value, (int, float)):
        if math.isnan(value):
            return None
        return float(value)
    text = str(value).strip()
    if not text or text.lower() in {"none", "nan", "null"}:
        return None
    try:
        parsed = float(text)
    except ValueError:
        return None
    if math.isnan(parsed):
        return None
    return parsed


def find_column(columns: Iterable[str], needles: Iterable[str]) -> str | None:
    lowered = {column.lower(): column for column in columns}
    for needle in needles:
        needle_lower = needle.lower()
        for lowered_name, original_name in lowered.items():
            if needle_lower in lowered_name:
                return original_name
    return None


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def svg_template(title: str, subtitle: str | None = None) -> list[str]:
    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{CANVAS_WIDTH}" height="{CANVAS_HEIGHT}" viewBox="0 0 {CANVAS_WIDTH} {CANVAS_HEIGHT}">',
        '<rect width="100%" height="100%" fill="#0b1020"/>',
        f'<text x="{MARGIN_LEFT}" y="42" fill="#f4f7fb" font-family="Segoe UI, Arial, sans-serif" font-size="28" font-weight="700">{escape(title)}</text>',
    ]
    if subtitle:
        lines.append(
            f'<text x="{MARGIN_LEFT}" y="66" fill="#aab4c3" font-family="Segoe UI, Arial, sans-serif" font-size="14">{escape(subtitle)}</text>'
        )
    return lines


def svg_close() -> list[str]:
    return ["</svg>"]


def axis_transform(x_index: int, y_value: float, x_count: int, y_min: float, y_max: float) -> tuple[float, float]:
    usable_width = max(1.0, float(PLOT_WIDTH))
    usable_height = max(1.0, float(PLOT_HEIGHT))
    if x_count <= 1:
        x_pos = MARGIN_LEFT + usable_width / 2.0
    else:
        x_pos = MARGIN_LEFT + (x_index / (x_count - 1)) * usable_width
    if y_max == y_min:
        y_pos = MARGIN_TOP + usable_height / 2.0
    else:
        y_pos = MARGIN_TOP + (1.0 - (y_value - y_min) / (y_max - y_min)) * usable_height
    return x_pos, y_pos


def write_svg(path: Path, lines: list[str]) -> None:
    path.write_text("\n".join(lines), encoding="utf-8")


def draw_axes(lines: list[str], x_labels: list[str], y_min: float, y_max: float, y_label: str) -> None:
    plot_left = MARGIN_LEFT
    plot_top = MARGIN_TOP
    plot_right = MARGIN_LEFT + PLOT_WIDTH
    plot_bottom = MARGIN_TOP + PLOT_HEIGHT

    lines.append(f'<rect x="{plot_left}" y="{plot_top}" width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" fill="#11182b" stroke="#263147" stroke-width="1" rx="14"/>')
    lines.append(f'<line x1="{plot_left}" y1="{plot_bottom}" x2="{plot_right}" y2="{plot_bottom}" stroke="#5c6b82" stroke-width="2"/>')
    lines.append(f'<line x1="{plot_left}" y1="{plot_top}" x2="{plot_left}" y2="{plot_bottom}" stroke="#5c6b82" stroke-width="2"/>')
    lines.append(f'<text x="18" y="{MARGIN_TOP + PLOT_HEIGHT / 2}" fill="#b5c1d4" font-family="Segoe UI, Arial, sans-serif" font-size="14" transform="rotate(-90 18 {MARGIN_TOP + PLOT_HEIGHT / 2})">{escape(y_label)}</text>')

    tick_count = 5
    for tick in range(tick_count + 1):
        y_value = y_min + (y_max - y_min) * (tick / tick_count)
        _, y_pos = axis_transform(0, y_value, max(2, len(x_labels)), y_min, y_max)
        lines.append(f'<line x1="{plot_left - 6}" y1="{y_pos:.2f}" x2="{plot_left}" y2="{y_pos:.2f}" stroke="#5c6b82" stroke-width="1"/>')
        lines.append(f'<text x="{plot_left - 12}" y="{y_pos + 4:.2f}" fill="#b5c1d4" font-family="Segoe UI, Arial, sans-serif" font-size="12" text-anchor="end">{y_value:.2f}</text>')

    label_slots = min(len(x_labels), 12)
    if label_slots == 0:
        label_slots = 1
    step = max(1, len(x_labels) // label_slots)
    for idx, label in enumerate(x_labels):
        if idx % step != 0 and idx != len(x_labels) - 1:
            continue
        x_pos, _ = axis_transform(idx, y_min, len(x_labels), y_min, y_max)
        lines.append(f'<line x1="{x_pos:.2f}" y1="{plot_bottom}" x2="{x_pos:.2f}" y2="{plot_bottom + 6}" stroke="#5c6b82" stroke-width="1"/>')
        text = escape(label)
        lines.append(f'<text x="{x_pos:.2f}" y="{plot_bottom + 24}" fill="#b5c1d4" font-family="Segoe UI, Arial, sans-serif" font-size="12" text-anchor="middle" transform="rotate(-35 {x_pos:.2f} {plot_bottom + 24})">{text}</text>')


def plot_line_chart(
    out_path: Path,
    title: str,
    subtitle: str,
    x_labels: list[str],
    series_map: OrderedDict[str, list[float | None]],
    y_label: str,
) -> bool:
    valid_series = OrderedDict((name, [value for value in values if value is not None]) for name, values in series_map.items())
    if not x_labels or not any(values for values in valid_series.values()):
        return False

    numeric_values = [value for values in series_map.values() for value in values if value is not None]
    if not numeric_values:
        return False

    y_min = min(numeric_values)
    y_max = max(numeric_values)
    if y_min == y_max:
        padding = 1.0 if y_min == 0 else abs(y_min) * 0.1
        y_min -= padding
        y_max += padding
    else:
        padding = (y_max - y_min) * 0.1
        y_min -= padding
        y_max += padding

    lines = svg_template(title, subtitle)
    draw_axes(lines, x_labels, y_min, y_max, y_label)

    for series_index, (series_name, values) in enumerate(series_map.items()):
        points: list[str] = []
        color = PALETTE[series_index % len(PALETTE)]
        for idx, value in enumerate(values):
            if value is None:
                continue
            x_pos, y_pos = axis_transform(idx, value, len(x_labels), y_min, y_max)
            points.append(f"{x_pos:.2f},{y_pos:.2f}")
            lines.append(f'<circle cx="{x_pos:.2f}" cy="{y_pos:.2f}" r="3.5" fill="{color}" stroke="#ffffff" stroke-width="1"/>')
        if points:
            lines.append(f'<polyline fill="none" stroke="{color}" stroke-width="3" stroke-linejoin="round" stroke-linecap="round" points="{" ".join(points)}"/>')

    legend_x = MARGIN_LEFT + 20
    legend_y = 88
    for idx, (series_name, _) in enumerate(series_map.items()):
        color = PALETTE[idx % len(PALETTE)]
        entry_y = legend_y + idx * 22
        lines.append(f'<rect x="{legend_x}" y="{entry_y - 12}" width="12" height="12" rx="3" fill="{color}"/>')
        lines.append(f'<text x="{legend_x + 20}" y="{entry_y - 2}" fill="#e9eef6" font-family="Segoe UI, Arial, sans-serif" font-size="13">{escape(series_name)}</text>')

    lines.extend(svg_close())
    write_svg(out_path, lines)
    return True


def plot_bar_chart(
    out_path: Path,
    title: str,
    subtitle: str,
    labels: list[str],
    values: list[float],
    y_label: str,
    color: str = "#4f8cff",
) -> bool:
    if not labels or not values:
        return False
    numeric = [value for value in values if value is not None]
    if not numeric:
        return False

    y_max = max(max(numeric), 1.0)
    lines = svg_template(title, subtitle)
    draw_axes(lines, labels, 0.0, y_max * 1.15, y_label)

    bar_space = PLOT_WIDTH / max(1, len(labels))
    bar_width = min(60.0, bar_space * 0.55)
    for idx, (label, value) in enumerate(zip(labels, values, strict=False)):
        if value is None:
            continue
        x_pos, y_pos = axis_transform(idx, value, len(labels), 0.0, y_max * 1.15)
        bar_height = MARGIN_TOP + PLOT_HEIGHT - y_pos
        bar_x = x_pos - bar_width / 2.0
        lines.append(f'<rect x="{bar_x:.2f}" y="{y_pos:.2f}" width="{bar_width:.2f}" height="{bar_height:.2f}" rx="8" fill="{color}" opacity="0.92"/>')
        lines.append(f'<text x="{x_pos:.2f}" y="{y_pos - 8:.2f}" fill="#f4f7fb" font-family="Segoe UI, Arial, sans-serif" font-size="12" text-anchor="middle">{value:.2f}</text>')

    lines.extend(svg_close())
    write_svg(out_path, lines)
    return True


def color_for_value(value: float, min_value: float, max_value: float) -> str:
    if max_value == min_value:
        ratio = 0.5
    else:
        ratio = (value - min_value) / (max_value - min_value)
    ratio = max(0.0, min(1.0, ratio))
    red = int(32 + (249 - 32) * ratio)
    green = int(58 + (118 - 58) * ratio)
    blue = int(120 + (255 - 120) * ratio)
    return f"rgb({red},{green},{blue})"


def plot_heatmap(
    out_path: Path,
    title: str,
    subtitle: str,
    matrix: list[list[float]],
    row_labels: list[str],
    col_labels: list[str],
) -> bool:
    if not matrix or not matrix[0]:
        return False
    flattened = [value for row in matrix for value in row]
    if not flattened:
        return False

    min_value = min(flattened)
    max_value = max(flattened)
    rows = len(matrix)
    cols = len(matrix[0])
    cell_width = PLOT_WIDTH / cols
    cell_height = PLOT_HEIGHT / rows

    lines = svg_template(title, subtitle)
    lines.append(f'<rect x="{MARGIN_LEFT}" y="{MARGIN_TOP}" width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" fill="#11182b" stroke="#263147" stroke-width="1" rx="14"/>')

    for row_index, row in enumerate(matrix):
        for col_index, value in enumerate(row):
            fill = color_for_value(value, min_value, max_value)
            x = MARGIN_LEFT + col_index * cell_width
            y = MARGIN_TOP + row_index * cell_height
            lines.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{cell_width:.2f}" height="{cell_height:.2f}" fill="{fill}" stroke="#0b1020" stroke-width="1"/>')
            lines.append(
                f'<text x="{x + cell_width / 2:.2f}" y="{y + cell_height / 2 + 5:.2f}" fill="#ffffff" font-family="Segoe UI, Arial, sans-serif" font-size="14" text-anchor="middle" font-weight="700">{value:.0f}</text>'
            )

    for col_index, label in enumerate(col_labels or [str(idx) for idx in range(cols)]):
        x = MARGIN_LEFT + col_index * cell_width + cell_width / 2
        lines.append(f'<text x="{x:.2f}" y="{MARGIN_TOP - 12}" fill="#b5c1d4" font-family="Segoe UI, Arial, sans-serif" font-size="12" text-anchor="middle">{escape(label)}</text>')
    for row_index, label in enumerate(row_labels or [str(idx) for idx in range(rows)]):
        y = MARGIN_TOP + row_index * cell_height + cell_height / 2 + 5
        lines.append(f'<text x="{MARGIN_LEFT - 10}" y="{y:.2f}" fill="#b5c1d4" font-family="Segoe UI, Arial, sans-serif" font-size="12" text-anchor="end">{escape(label)}</text>')

    lines.extend(svg_close())
    write_svg(out_path, lines)
    return True


def extract_results_series(rows: list[dict[str, str]]) -> tuple[list[str], OrderedDict[str, list[float | None]], OrderedDict[str, list[float | None]]]:
    if not rows:
        return [], OrderedDict(), OrderedDict()

    columns = list(rows[0].keys())
    epoch_col = find_column(columns, ["epoch"]) or columns[0]
    x_labels = [row.get(epoch_col, str(index)) for index, row in enumerate(rows)]

    loss_candidates = OrderedDict(
        [
            ("train/box_loss", ["train/box_loss", "train_box_loss", "box_loss"]),
            ("train/cls_loss", ["train/cls_loss", "train_cls_loss", "cls_loss"]),
            ("train/dfl_loss", ["train/dfl_loss", "train_dfl_loss", "dfl_loss"]),
            ("val/box_loss", ["val/box_loss", "val_box_loss"]),
            ("val/cls_loss", ["val/cls_loss", "val_cls_loss"]),
            ("val/dfl_loss", ["val/dfl_loss", "val_dfl_loss"]),
        ]
    )
    metric_candidates = OrderedDict(
        [
            ("precision", ["precision"]),
            ("recall", ["recall"]),
            ("mAP50", ["map50", "mAP50"]),
            ("mAP50-95", ["map50-95", "mAP50-95", "map50_95"]),
        ]
    )

    def build_series(candidates: OrderedDict[str, list[str]]) -> OrderedDict[str, list[float | None]]:
        series: OrderedDict[str, list[float | None]] = OrderedDict()
        for display_name, needle_list in candidates.items():
            column = find_column(columns, needle_list)
            if not column:
                continue
            values = [safe_float(row.get(column)) for row in rows]
            if any(value is not None for value in values):
                series[display_name] = values
        return series

    return x_labels, build_series(loss_candidates), build_series(metric_candidates)


def parse_matrix_input(path: Path) -> tuple[list[list[float]], list[str], list[str]]:
    if path.suffix.lower() == ".json":
        payload = read_json(path)
        if isinstance(payload, dict):
            matrix = payload.get("matrix") or payload.get("data") or payload.get("values")
            if not matrix:
                raise ValueError("JSON confusion input must contain a 'matrix', 'data', or 'values' field.")
            row_labels = [str(item) for item in payload.get("labels") or payload.get("row_labels") or payload.get("classes") or []]
            col_labels = [str(item) for item in payload.get("labels") or payload.get("col_labels") or payload.get("classes") or []]
            parsed_matrix = [[float(value) for value in row] for row in matrix]
            return parsed_matrix, row_labels, col_labels
        raise ValueError("Unsupported JSON confusion input format.")

    rows = read_csv_rows(path)
    if not rows:
        raise ValueError("Confusion matrix CSV is empty.")
    header = list(rows[0].keys())
    matrix: list[list[float]] = []
    for row in rows:
        matrix.append([safe_float(row.get(column)) or 0.0 for column in header])
    labels = [column for column in header]
    return matrix, labels, labels


def build_runtime_graphs(runtime_path: Path, out_dir: Path) -> list[Path]:
    created: list[Path] = []
    payload = read_json(runtime_path)
    counts = payload.get("counts", {}) if isinstance(payload, dict) else {}
    count_labels = ["telemetry", "heartbeat", "violations", "cloud_sync", "parse_errors"]
    count_values = [float(counts.get(label, 0) or 0) for label in count_labels]
    out_path = out_dir / "runtime_event_counts.svg"
    if plot_bar_chart(
        out_path,
        "Runtime Event Counts",
        "Captured events over the telemetry window",
        count_labels,
        count_values,
        "messages",
        color="#ff8c42",
    ):
        created.append(out_path)

    stream_fps = payload.get("stream_fps", {}) if isinstance(payload, dict) else {}
    if isinstance(stream_fps, dict) and stream_fps:
        labels = [str(key) for key in stream_fps.keys()]
        values = [safe_float(value) or 0.0 for value in stream_fps.values()]
        out_path = out_dir / "runtime_stream_fps.svg"
        if plot_bar_chart(
            out_path,
            "Per-Stream FPS",
            "Average FPS reported during the capture window",
            labels,
            values,
            "fps",
            color="#4f8cff",
        ):
            created.append(out_path)

    latency = payload.get("latency_ms", {}) if isinstance(payload, dict) else {}
    if isinstance(latency, dict) and latency:
        labels = [str(key) for key in latency.keys()]
        values = [safe_float(value) or 0.0 for value in latency.values()]
        out_path = out_dir / "runtime_latency_ms.svg"
        if plot_bar_chart(
            out_path,
            "Latency Statistics",
            "Latency values reported during the capture window",
            labels,
            values,
            "ms",
            color="#9c6bff",
        ):
            created.append(out_path)

    gpu = payload.get("gpu", {}) if isinstance(payload, dict) else {}
    if isinstance(gpu, dict) and gpu:
        labels = ["utilization_percent", "temperature_c", "memory_used_mb", "memory_total_mb"]
        values = [safe_float(gpu.get(label)) or 0.0 for label in labels]
        out_path = out_dir / "runtime_gpu.svg"
        if plot_bar_chart(
            out_path,
            "GPU Telemetry",
            "GPU stats captured during the window; zeros indicate no telemetry received",
            labels,
            values,
            "value",
            color="#22c55e",
        ):
            created.append(out_path)

    return created


def build_db_graphs(db_path: Path, out_dir: Path) -> list[Path]:
    created: list[Path] = []
    payload = read_json(db_path)
    violations = payload.get("violations", {}) if isinstance(payload, dict) else {}
    summary = violations.get("summary", {}) if isinstance(violations, dict) else {}

    summary_labels = ["total", "uploaded_count", "pending_count"]
    summary_values = [float(summary.get(label, 0) or 0) for label in summary_labels]
    out_path = out_dir / "db_violation_summary.svg"
    if plot_bar_chart(
        out_path,
        "Violation Summary",
        "Current persisted violation totals",
        summary_labels,
        summary_values,
        "records",
        color="#ff4f6d",
    ):
        created.append(out_path)

    by_day = violations.get("by_day_last_30", []) if isinstance(violations, dict) else []
    if isinstance(by_day, list) and by_day:
        labels = [str(item.get("day", idx)) for idx, item in enumerate(by_day)]
        values = [float(item.get("count", 0) or 0) for item in by_day]
        out_path = out_dir / "db_violations_by_day.svg"
        if plot_bar_chart(
            out_path,
            "Violations by Day",
            "Counts from the last 30 days",
            labels,
            values,
            "count",
            color="#fb923c",
        ):
            created.append(out_path)

    by_zone = violations.get("by_zone", []) if isinstance(violations, dict) else []
    if isinstance(by_zone, list) and by_zone:
        labels = [f"Zone {item.get('zone_id', idx)}" for idx, item in enumerate(by_zone)]
        values = [float(item.get("count", 0) or 0) for item in by_zone]
        out_path = out_dir / "db_violations_by_zone.svg"
        if plot_bar_chart(
            out_path,
            "Violations by Zone",
            "Distribution of recorded violations across zones",
            labels,
            values,
            "count",
            color="#06b6d4",
        ):
            created.append(out_path)

    by_camera = violations.get("by_camera", []) if isinstance(violations, dict) else []
    if isinstance(by_camera, list) and by_camera:
        labels = [str(item.get("camera_id", idx)) for idx, item in enumerate(by_camera)]
        values = [float(item.get("count", 0) or 0) for item in by_camera]
        out_path = out_dir / "db_violations_by_camera.svg"
        if plot_bar_chart(
            out_path,
            "Violations by Camera",
            "Distribution of recorded violations across cameras",
            labels,
            values,
            "count",
            color="#a855f7",
        ):
            created.append(out_path)

    latest = violations.get("latest_10", []) if isinstance(violations, dict) else []
    if isinstance(latest, list) and latest:
        labels = [str(item.get("timestamp", idx)) for idx, item in enumerate(latest)]
        values = [float(item.get("confidence", 0) or 0) for item in latest]
        out_path = out_dir / "db_confidence_latest.svg"
        if plot_bar_chart(
            out_path,
            "Latest Violation Confidence",
            "Confidence values for the most recent records",
            labels,
            values,
            "confidence",
            color="#22c55e",
        ):
            created.append(out_path)

    return created


def build_training_graphs(results_path: Path, out_dir: Path) -> list[Path]:
    created: list[Path] = []
    rows = read_csv_rows(results_path)
    x_labels, loss_series, metric_series = extract_results_series(rows)
    if x_labels and loss_series:
        out_path = out_dir / "training_losses.svg"
        if plot_line_chart(
            out_path,
            "Training and Validation Losses",
            f"Source: {results_path.name}",
            x_labels,
            loss_series,
            "loss",
        ):
            created.append(out_path)
    if x_labels and metric_series:
        out_path = out_dir / "training_metrics.svg"
        if plot_line_chart(
            out_path,
            "Precision, Recall, and mAP",
            f"Source: {results_path.name}",
            x_labels,
            metric_series,
            "score",
        ):
            created.append(out_path)
    return created


def build_confusion_graph(confusion_path: Path, out_dir: Path) -> Path | None:
    matrix, row_labels, col_labels = parse_matrix_input(confusion_path)
    out_path = out_dir / "confusion_matrix.svg"
    if plot_heatmap(
        out_path,
        "Confusion Matrix",
        f"Source: {confusion_path.name}",
        matrix,
        row_labels,
        col_labels,
    ):
        return out_path
    return None


def build_report_index(out_dir: Path, generated_files: list[Path], skipped_notes: list[str]) -> Path:
    report = [
        "<!doctype html>",
        "<html lang='en'>",
        "<head>",
        "<meta charset='utf-8'/>",
        "<meta name='viewport' content='width=device-width, initial-scale=1'/>",
        "<title>Project Graph Report</title>",
        "<style>",
        "body{margin:0;font-family:Segoe UI,Arial,sans-serif;background:#0b1020;color:#f4f7fb;}",
        "main{max-width:1280px;margin:0 auto;padding:24px;}",
        "h1{margin:0 0 8px 0;font-size:28px;}",
        "p,li{color:#cbd5e1;line-height:1.55;}",
        "section{margin:24px 0;padding:20px;background:#11182b;border:1px solid #263147;border-radius:16px;}",
        "img{max-width:100%;height:auto;display:block;background:#0b1020;border-radius:12px;border:1px solid #263147;}",
        "code{background:#0b1020;padding:2px 6px;border-radius:6px;}",
        "</style>",
        "</head>",
        "<body><main>",
        "<h1>Project Graph Report</h1>",
        "<p>Generated from the current repository snapshots and any training artifacts you provided.</p>",
    ]
    if skipped_notes:
        report.append("<section><h2>Notes</h2><ul>")
        for note in skipped_notes:
            report.append(f"<li>{escape(note)}</li>")
        report.append("</ul></section>")
    for file_path in generated_files:
        report.append("<section>")
        report.append(f"<h2>{escape(file_path.stem.replace('_', ' ').title())}</h2>")
        report.append(f"<img src='{escape(file_path.name)}' alt='{escape(file_path.stem)}'/>")
        report.append("</section>")
    report.extend(["</main></body></html>"])
    out_path = out_dir / "report.html"
    out_path.write_text("\n".join(report), encoding="utf-8")
    return out_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate SVG graphs for the project report.")
    parser.add_argument("--results", type=Path, help="Path to Ultralytics results.csv")
    parser.add_argument("--confusion", type=Path, help="Path to a confusion matrix JSON or CSV file")
    parser.add_argument("--runtime", type=Path, help="Path to metrics_runtime_snapshot.json")
    parser.add_argument("--db", type=Path, help="Path to metrics_db_snapshot.json")
    parser.add_argument("--outdir", type=Path, default=Path("docs") / "graphs", help="Directory to write the generated SVG files")
    args = parser.parse_args()

    ensure_dir(args.outdir)
    generated: list[Path] = []
    notes: list[str] = []

    if args.results:
        if args.results.exists():
            generated.extend(build_training_graphs(args.results, args.outdir))
        else:
            notes.append(f"Training results file not found: {args.results}")

    if args.confusion:
        if args.confusion.exists():
            try:
                graph = build_confusion_graph(args.confusion, args.outdir)
                if graph:
                    generated.append(graph)
            except Exception as exc:  # pragma: no cover - surfaced to the user via console output
                notes.append(f"Could not build confusion matrix graph: {exc}")
        else:
            notes.append(f"Confusion matrix input not found: {args.confusion}")
    else:
        notes.append("No confusion matrix input was provided. Supply a JSON or CSV matrix file to generate that graph.")

    if args.runtime:
        if args.runtime.exists():
            generated.extend(build_runtime_graphs(args.runtime, args.outdir))
        else:
            notes.append(f"Runtime snapshot not found: {args.runtime}")

    if args.db:
        if args.db.exists():
            generated.extend(build_db_graphs(args.db, args.outdir))
        else:
            notes.append(f"Database snapshot not found: {args.db}")

    if not generated:
        notes.append("No graph files were generated because the required input data was not present.")

    report_path = build_report_index(args.outdir, generated, notes)
    print(f"Generated {len(generated)} graph file(s) in {args.outdir}")
    for path in generated:
        print(f" - {path}")
    print(f"Report index: {report_path}")
    if notes:
        print("Notes:")
        for note in notes:
            print(f" - {note}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
