#!/usr/bin/env python3
"""Collect a coherent original draw-task hierarchy from the opt-in native trace.

Matrices are relative render-task transforms, not authenticated anatomical
joint pivots. No game memory is read or changed by this offline tool.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np


def matrix(words):
    if len(words) != 16 or any(type(w) is not int or not 0 <= w <= 0xffffffff for w in words):
        raise ValueError('Missing or malformed fixed-point matrix')
    data = struct.pack('>16I', *words)
    integers = np.frombuffer(data[:32], dtype='>i2').astype(np.float64)
    fractions = np.frombuffer(data[32:], dtype='>u2').astype(np.float64)
    return (integers + fractions / 65536).reshape(4, 4)


def pose(records, minimum=2):
    if not records or len({r['player_draw_epoch'] for r in records}) != 1 or len({r['update_counter'] for r in records}) != 1:
        raise ValueError('Do not combine different player draws or raw counters')
    if any(len(r['task_words']) != 12 for r in records):
        raise ValueError('Missing task topology')
    nodes = {r['task']: r for r in records}
    if len(nodes) != len(records):
        raise ValueError('Ambiguous duplicate task in one update')
    roots = [r for r in records if r['is_player_candidate']]
    if len(roots) != 1:
        raise ValueError('Require one observed original player root')
    root = roots[0]
    selected = {root['task']}
    for _ in range(32):
        added = {r['task'] for r in records if r['task_words'][3] in selected} - selected
        if not added:
            break
        selected.update(added)
    else:
        raise ValueError('Unbounded hierarchy')
    if len(selected) < minimum:
        raise ValueError('Not enough observed player draw tasks')
    for task in selected:
        row = nodes[task]
        for index in (2, 4):  # original first/last child pointers
            child = row['task_words'][index]
            if child and (child not in selected or nodes[child]['task_words'][3] != task):
                raise ValueError('Incomplete or inconsistent child hierarchy')
        parent = row['task_words'][3]
        if task == root['task']:
            if parent in selected:
                raise ValueError('Player root participates in a parent cycle')
        elif parent not in selected:
            raise ValueError('Missing player ancestor')
    root_matrix = matrix(root['matrix_words'])
    try:
        inverse = np.linalg.inv(root_matrix)
    except np.linalg.LinAlgError as error:
        raise ValueError('Singular player transform') from error
    parts = []
    for task in sorted(selected):
        row = nodes[task]
        relative = matrix(row['matrix_words']) @ inverse
        if not np.isfinite(relative).all() or np.linalg.det(relative) == 0:
            raise ValueError('Invalid relative task transform')
        parts.append({'task': task, 'parent': row['task_words'][3],
                      'relative_transform': relative.tolist()})
    return {'player_draw_epoch': root['player_draw_epoch'], 'raw_original_counter': root['update_counter'], 'player_task': root['task'],
            'parts': parts, 'anatomical_joint_mapping_verified': False,
            'nomercy_retargeting_verified': False}


def collect(path, minimum):
    selected = None
    group = []
    # Parse and fingerprint the same immutable bytes; a live append must not
    # make provenance refer to a different trace from the one inspected.
    data = path.read_bytes()
    for line in data.splitlines(keepends=True):
        if not line.endswith(b'\n'):
            break  # A live append can be incomplete; never use its partial data.
        row = json.loads(line)
        counter, epoch = row['update_counter'], row['player_draw_epoch']
        if type(counter) is not int or not 0 <= counter <= 65535:
            raise ValueError('Invalid original raw counter')
        if type(epoch) is not int or not 0 <= epoch <= 0xffffffffffffffff:
            raise ValueError('Invalid player draw epoch')
        if group and epoch != group[0]['player_draw_epoch']:
            try:
                selected = pose(group, minimum)
            except ValueError:
                pass  # Sparse research sampling can miss an ancestor.
            group = []
        group.append(row)
    # The last group may still be receiving children from the live process.
    # Only a following draw epoch establishes a completed observation group.
    if selected is None:
        raise ValueError('No completed, coherent player hierarchy in this trace')
    selected['source_trace_sha256'] = hashlib.sha256(data).hexdigest()
    return selected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--minimum', type=int, default=6)
    args = parser.parse_args()
    if not 2 <= args.minimum <= 128:
        parser.error('Minimum must be between 2 and 128 draw tasks')
    if args.output.exists():
        parser.error('Choose a fresh output path')
    try:
        result = collect(args.trace, args.minimum)
        with args.output.open('x') as stream:
            stream.write(json.dumps(result, indent=2) + '\n')
    except (ValueError, KeyError, OSError, json.JSONDecodeError) as error:
        parser.exit(1, f'Pose not collected: {error}\n')
    print(f"Collected {len(result['parts'])} original draw tasks; anatomical mapping remains unverified.")


if __name__ == '__main__':
    main()
