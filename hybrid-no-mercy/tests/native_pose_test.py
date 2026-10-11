#!/usr/bin/env python3
"""Verify relative transforms, coherent counters and incomplete-hierarchy rejection."""
from copy import deepcopy
import json
from pathlib import Path
import struct
import sys
import tempfile
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from collect_native_pose import matrix, pose, collect


def words(m):
    flat = m.reshape(-1)
    integer = np.floor(flat).astype('>i2').tobytes()
    fraction = np.rint((flat - np.floor(flat)) * 65536).astype('>u2').tobytes()
    return list(struct.unpack('>16I', integer + fraction))


root = np.eye(4); root[3, :3] = [10.5, 20.25, -30.75]
relative = np.eye(4); relative[3, :3] = [5.5, -2.25, 3.125]
records = []
for task, parent, child, m in [(0x80010000, 0, 0x80010100, root),
                             (0x80010100, 0x80010000, 0, relative @ root)]:
    topology = [0] * 12; topology[2] = topology[4] = child; topology[3] = parent
    records.append({'task': task, 'task_words': topology, 'matrix_words': words(m),
                    'is_player_candidate': parent == 0, 'update_counter': 65535, 'player_draw_epoch': 10})
np.testing.assert_array_equal(matrix(words(root)), root)
result = pose(records)
np.testing.assert_allclose(result['parts'][0]['relative_transform'], np.eye(4), rtol=0, atol=1e-12)
np.testing.assert_allclose(result['parts'][1]['relative_transform'], relative, rtol=0, atol=1e-12)
assert not result['anatomical_joint_mapping_verified']
assert not result['nomercy_retargeting_verified']

for change in ('counter', 'epoch', 'missing_child', 'duplicate', 'cycle', 'null_matrix', 'zero_root'):
    bad = deepcopy(records)
    if change == 'counter': bad[1]['update_counter'] = 0
    elif change == 'epoch': bad[1]['player_draw_epoch'] = 11
    elif change == 'missing_child': bad[1]['task_words'][2] = 0x80010200
    elif change == 'duplicate': bad.append(deepcopy(bad[1]))
    elif change == 'cycle': bad[0]['task_words'][3] = bad[1]['task']
    elif change == 'null_matrix': bad[1]['matrix_words'][0] = None
    elif change == 'zero_root': bad[0]['matrix_words'] = [0] * 16
    try: pose(bad)
    except ValueError: pass
    else: raise AssertionError(f'Accepted {change}')

with tempfile.TemporaryDirectory() as folder:
    p = Path(folder) / 'trace.jsonl'
    p.write_text(''.join(json.dumps(r) + '\n' for r in records))
    try: collect(p, 2)
    except ValueError: pass
    else: raise AssertionError('Accepted the potentially incomplete live tail group')
    later = deepcopy(records[0]); later['player_draw_epoch'] = 11
    with p.open('a') as stream: stream.write(json.dumps(later) + '\n')
    assert collect(p, 2)['raw_original_counter'] == 65535
    with p.open('a') as stream: stream.write('{"update_counter":')
    assert collect(p, 2)['raw_original_counter'] == 65535
print('PASS: native fixed-point endian decoding, relative transforms and coherent counter grouping.')
print('PASS: mixed counters, missing children, duplicate tasks, cycles, bad matrices and live-tail rejection.')
