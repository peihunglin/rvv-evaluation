#!/usr/bin/env python3

import argparse
import csv
import re
from pathlib import Path


RATE_RE = re.compile(r"Rate \(MFlops/s\):\s+([-+0-9.eE]+)\s+Avg time \(s\):\s+([-+0-9.eE]+)")
ELAPSED_RE = re.compile(r"\s*([-+0-9.]+)\s+seconds time elapsed")
COUNTER_RE = re.compile(r"\s*([0-9,]+)\s+(instructions|L1-dcache-loads|L1-dcache-load-misses):u")
KV_RE = re.compile(r"^([^=]+?)\s*=\s*(.*)$")
METADATA_RE = re.compile(
    r"^(Matrix order|Number of iterations|Blocking factor|IME tile size|Number of threads|"
    r"X100 threads|A100 threads|A100 column share|Precision|RVV LMUL)\s*=\s*(.+)$"
)


FIELDNAMES = [
    "name",
    "series",
    "family",
    "mode",
    "compiler",
    "precision",
    "precision_group",
    "core",
    "variant",
    "lmul",
    "mc",
    "nc",
    "nr",
    "ku",
    "kc",
    "threads",
    "x100_threads",
    "a100_threads",
    "a100_share",
    "matrix_order",
    "iterations",
    "tile_size",
    "status",
    "mflops",
    "avg_time_s",
    "elapsed_s",
    "instructions",
    "l1_loads",
    "l1_load_misses",
    "command",
    "file",
]


def parse_number(text):
    return text.replace(",", "")


def precision_group(precision):
    if precision == "double":
        return "fp64"
    if precision == "float":
        return "fp32"
    if precision == "fp16":
        return "fp16"
    return precision or "unknown"


def strip_prefix(value, prefix):
    return value[len(prefix):] if value.startswith(prefix) else value


def set_from_metadata(row, key, value):
    key = key.strip()
    value = value.strip()
    mapping = {
        "Matrix order": "matrix_order",
        "Number of iterations": "iterations",
        "Blocking factor": "tile_size",
        "IME tile size": "tile_size",
        "Number of threads": "threads",
        "X100 threads": "x100_threads",
        "A100 threads": "a100_threads",
        "A100 column share": "a100_share",
        "Precision": "precision",
        "RVV LMUL": "lmul",
    }
    field = mapping.get(key)
    if field:
        if field == "lmul":
            value = value.lstrip("m")
        row[field] = value


def classify_from_name(row):
    name = row["name"]
    parts = name.split("_")

    row["series"] = "baseline"

    if name.startswith("opt_gemm_"):
        row["series"] = "optimized"
        row["family"] = "opt_base"
        row["mode"] = "opt_gemm"
        row["compiler"] = parts[2]
        row["precision"] = parts[3]
        if len(parts) >= 8 and parts[4].startswith("nr"):
            row["nr"] = strip_prefix(parts[4], "nr")
            row["kc"] = strip_prefix(parts[5], "kc")
            row["threads"] = strip_prefix(parts[6], "t")
            row["matrix_order"] = strip_prefix(parts[7], "n")
        else:
            row["threads"] = strip_prefix(parts[4], "t")
            row["matrix_order"] = strip_prefix(parts[5], "n")
        row["variant"] = "gemm_opt"
    elif name.startswith("opt_rvv_x100_") or name.startswith("opt_rvv_a100_"):
        row["series"] = "optimized"
        row["family"] = "opt_rvv"
        row["mode"] = "opt_rvv"
        row["core"] = parts[2]
        row["compiler"] = parts[3]
        row["precision"] = parts[4]
        if len(parts) >= 12 and parts[5].startswith("mc"):
            row["mc"] = strip_prefix(parts[5], "mc")
            row["nc"] = strip_prefix(parts[6], "nc")
            row["nr"] = strip_prefix(parts[7], "nr")
            row["ku"] = strip_prefix(parts[8], "ku")
            row["kc"] = strip_prefix(parts[9], "kc")
            row["threads"] = strip_prefix(parts[10], "t")
            row["matrix_order"] = strip_prefix(parts[11], "n")
        else:
            row["threads"] = strip_prefix(parts[5], "t")
            row["matrix_order"] = strip_prefix(parts[6], "n")
        row["variant"] = f"rvv_{row['core']}_opt"
    elif name.startswith("opt_ime_"):
        row["series"] = "optimized"
        row["family"] = "opt_ime"
        row["mode"] = "opt_ime"
        row["compiler"] = parts[2]
        row["precision"] = parts[3]
        row["threads"] = strip_prefix(parts[4], "t")
        row["matrix_order"] = strip_prefix(parts[5], "n")
        row["core"] = "a100"
        row["variant"] = "ime_opt"
    elif name.startswith("gemm_hetero_rvv_ime_"):
        row["family"] = "hetero"
        row["mode"] = "rvv_ime"
        row["lmul"] = row["lmul"] or parts[4]
        row["compiler"] = parts[5]
        row["precision"] = parts[6]
        row["x100_threads"] = strip_prefix(parts[7], "x")
        row["a100_threads"] = strip_prefix(parts[8], "a")
        row["a100_share"] = parts[9].replace("share", "")
        row["matrix_order"] = strip_prefix(parts[10], "n")
        row["variant"] = f"rvv_ime_m{row['lmul']}"
    elif name.startswith("gemm_hetero_rvv_rvv_"):
        row["family"] = "hetero"
        row["mode"] = "rvv_rvv"
        row["lmul"] = row["lmul"] or parts[4]
        row["compiler"] = parts[5]
        row["precision"] = parts[6]
        row["x100_threads"] = strip_prefix(parts[7], "x")
        row["a100_threads"] = strip_prefix(parts[8], "a")
        row["a100_share"] = parts[9].replace("share", "")
        row["matrix_order"] = strip_prefix(parts[10], "n")
        row["variant"] = f"rvv_rvv_m{row['lmul']}"
    elif name.startswith("gemm_ime_"):
        row["family"] = "ime"
        row["mode"] = "ime"
        row["compiler"] = parts[2]
        row["precision"] = parts[3]
        row["threads"] = strip_prefix(parts[4], "t")
        row["matrix_order"] = strip_prefix(parts[5], "n")
        row["core"] = "a100"
        row["variant"] = "ime"
    elif name.startswith("gemm_rvv_"):
        row["family"] = "rvv"
        row["mode"] = "rvv"
        offset = 2
        if parts[2] in {"1", "2", "4", "8"}:
            row["lmul"] = row["lmul"] or parts[2]
            offset = 3
        else:
            row["lmul"] = row["lmul"] or "1"
        row["core"] = parts[offset]
        row["compiler"] = parts[offset + 1]
        row["precision"] = parts[offset + 2]
        row["threads"] = strip_prefix(parts[offset + 3], "t")
        row["matrix_order"] = strip_prefix(parts[offset + 4], "n")
        row["variant"] = f"rvv_m{row['lmul']}"
    elif name.startswith("gemm_"):
        row["family"] = "base"
        row["compiler"] = parts[1]
        row["precision"] = parts[2]
        if parts[3] == "autovec" and parts[4] in {"1", "2", "4", "8"}:
            row["mode"] = "autovec_lmul"
            row["lmul"] = parts[4]
            row["threads"] = strip_prefix(parts[5], "t")
            row["matrix_order"] = strip_prefix(parts[6], "n")
            row["variant"] = f"autovec_{row['lmul']}"
        else:
            row["mode"] = parts[3]
            row["threads"] = strip_prefix(parts[4], "t")
            row["matrix_order"] = strip_prefix(parts[5], "n")
            row["variant"] = row["mode"]


def parse_file(path):
    text = path.read_text(errors="replace")
    lines = text.splitlines()
    row = {field: "" for field in FIELDNAMES}
    row["name"] = path.stem
    row["file"] = str(path)
    row["command"] = lines[0] if lines else ""
    row["status"] = "unknown"

    classify_from_name(row)
    row["precision_group"] = precision_group(row["precision"])

    if "Solution validates" in text:
        row["status"] = "valid"
    if "ERROR: Checksum" in text:
        row["status"] = "checksum_error"
    elif "Illegal instruction" in text:
        row["status"] = "illegal_instruction"
    elif "ERROR:" in text and row["status"] == "unknown":
        row["status"] = "error"

    for line in lines:
        rate = RATE_RE.search(line)
        if rate:
            row["mflops"] = rate.group(1)
            row["avg_time_s"] = rate.group(2)

        elapsed = ELAPSED_RE.match(line)
        if elapsed:
            row["elapsed_s"] = elapsed.group(1)

        counter = COUNTER_RE.match(line)
        if counter:
            value, event = counter.groups()
            if event == "instructions":
                row["instructions"] = parse_number(value)
            elif event == "L1-dcache-loads":
                row["l1_loads"] = parse_number(value)
            elif event == "L1-dcache-load-misses":
                row["l1_load_misses"] = parse_number(value)

        if "=" in line and line.startswith("env "):
            for token in line.split():
                match = KV_RE.match(token)
                if match:
                    key, value = match.groups()
                    if key == "A100_SHARE":
                        row["a100_share"] = value
                    elif key == "X100_THREADS":
                        row["x100_threads"] = value
                    elif key == "A100_THREADS":
                        row["a100_threads"] = value

        metadata = METADATA_RE.match(line)
        if metadata:
            key, value = metadata.groups()
            set_from_metadata(row, key, value)

    row["precision_group"] = precision_group(row["precision"])

    return row


def sort_key(row):
    try:
        return float(row["mflops"])
    except ValueError:
        return -1.0


def write_csv(rows, path):
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)


def best_key(row):
    if row["family"] in {"hetero", "opt_ime", "ime"}:
        thread_key = (row["x100_threads"], row["a100_threads"], row["threads"], row["a100_share"])
    else:
        thread_key = ("", "", row["threads"], "")
    return (
        row["series"],
        row["family"],
        row["mode"],
        row["compiler"],
        row["precision_group"],
        row["precision"],
        row["core"],
        row["matrix_order"],
        *thread_key,
    )


def write_best_csv(rows, path):
    best = {}
    for row in rows:
        if row["status"] != "valid" or not row["mflops"]:
            continue
        key = best_key(row)
        if key not in best or sort_key(row) > sort_key(best[key]):
            best[key] = row

    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(sorted(best.values(), key=lambda row: (row["precision_group"], row["series"], row["family"], row["compiler"], row["name"])))


def write_hetero_csv(rows, path):
    hetero = [
        row for row in rows
        if row["status"] == "valid" and row["mflops"] and row["family"] == "hetero"
    ]
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(sorted(hetero, key=lambda row: (row["mode"], row["compiler"], row["precision"], row["variant"], int(row["a100_share"] or 0))))


def write_markdown(rows, path, limit):
    valid = [row for row in rows if row["status"] == "valid" and row["mflops"]]
    valid = sorted(valid, key=sort_key, reverse=True)
    failures = [row for row in rows if row["status"] != "valid"]
    precision_groups = ["fp64", "fp32", "fp16"]

    with path.open("w") as f:
        f.write("# GEMM Performance Summary\n\n")
        f.write(f"Parsed {len(rows)} result files. Valid: {len(valid)}. Non-valid: {len(failures)}.\n\n")

        for group in precision_groups:
            grouped = [row for row in valid if row["precision_group"] == group]
            if not grouped:
                continue
            f.write(f"## Top {min(limit, len(grouped))} {group} Runs By MFlops/s\n\n")
            f.write("| Rank | MFlops/s | Series | Family | Mode | Compiler | Precision | Core | Variant | LMUL | MC | NC | NR | KU | KC | Threads | A100 Share | File |\n")
            f.write("|---:|---:|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n")
            for idx, row in enumerate(grouped[:limit], 1):
                threads = row["threads"] or f"x{row['x100_threads']}/a{row['a100_threads']}"
                f.write(
                    f"| {idx} | {row['mflops']} | {row['series']} | {row['family']} | {row['mode']} | {row['compiler']} | "
                    f"{row['precision']} | {row['core']} | {row['variant']} | {row['lmul']} | "
                    f"{row['mc']} | {row['nc']} | {row['nr']} | {row['ku']} | {row['kc']} | {threads} | "
                    f"{row['a100_share']} | `{Path(row['file']).name}` |\n"
                )
            f.write("\n")

        other = [row for row in valid if row["precision_group"] not in precision_groups]
        if other:
            f.write(f"## Top {min(limit, len(other))} Other Precision Runs By MFlops/s\n\n")
            f.write("| Rank | MFlops/s | Series | Family | Mode | Compiler | Precision | Core | Variant | LMUL | MC | NC | NR | KU | KC | Threads | A100 Share | File |\n")
            f.write("|---:|---:|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n")
            for idx, row in enumerate(other[:limit], 1):
                threads = row["threads"] or f"x{row['x100_threads']}/a{row['a100_threads']}"
                f.write(
                    f"| {idx} | {row['mflops']} | {row['series']} | {row['family']} | {row['mode']} | {row['compiler']} | "
                    f"{row['precision']} | {row['core']} | {row['variant']} | {row['lmul']} | "
                    f"{row['mc']} | {row['nc']} | {row['nr']} | {row['ku']} | {row['kc']} | {threads} | "
                    f"{row['a100_share']} | `{Path(row['file']).name}` |\n"
                )
            f.write("\n")

        if failures:
            f.write("\n## Non-Valid Runs\n\n")
            f.write("| Status | Series | Family | Mode | Compiler | Precision | Variant | A100 Share | File |\n")
            f.write("|---|---|---|---|---|---|---|---|---|\n")
            for row in failures:
                f.write(
                    f"| {row['status']} | {row['series']} | {row['family']} | {row['mode']} | {row['compiler']} | "
                    f"{row['precision']} | {row['variant']} | {row['a100_share']} | `{Path(row['file']).name}` |\n"
                )


def main():
    parser = argparse.ArgumentParser(description="Extract GEMM benchmark result summaries.")
    parser.add_argument("results_dir", nargs="?", default="results", help="Directory containing run.sh .txt logs")
    parser.add_argument("--csv", default="results_summary.csv", help="CSV output path")
    parser.add_argument("--md", default="results_summary.md", help="Markdown summary output path")
    parser.add_argument("--best-csv", default="results_best.csv", help="Best-per-configuration CSV output path")
    parser.add_argument("--hetero-csv", default="results_hetero_share.csv", help="Hetero share-sweep CSV output path")
    parser.add_argument("--top", type=int, default=30, help="Number of valid runs to show in Markdown ranking")
    args = parser.parse_args()

    results_dir = Path(args.results_dir)
    rows = [parse_file(path) for path in sorted(results_dir.glob("*.txt"))]
    rows.sort(key=lambda row: (row["family"], row["mode"], row["compiler"], row["precision"], row["name"]))

    write_csv(rows, Path(args.csv))
    write_best_csv(rows, Path(args.best_csv))
    write_hetero_csv(rows, Path(args.hetero_csv))
    write_markdown(rows, Path(args.md), args.top)

    valid = sum(1 for row in rows if row["status"] == "valid")
    print(f"Parsed {len(rows)} files: {valid} valid, {len(rows) - valid} non-valid")
    print(f"Wrote {args.csv}")
    print(f"Wrote {args.best_csv}")
    print(f"Wrote {args.hetero_csv}")
    print(f"Wrote {args.md}")


if __name__ == "__main__":
    main()
