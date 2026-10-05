"""Explicit file identity/path checks. Does not certify tests, licenses or operator actions."""
import argparse
import hashlib
import json
import pathlib


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def inventory(root):
    root = root.resolve(strict=True)
    entries = []
    for p in sorted(root.rglob('*')):
        if p.is_symlink() or p.is_junction():
            raise ValueError('Linked artifact entries are forbidden')
        if p.is_file():
            entries.append({'path': p.relative_to(root).as_posix(),
                            'size': p.stat().st_size, 'sha256': sha256(p)})
    return entries


def identity(entries):
    return hashlib.sha256(json.dumps(entries, ensure_ascii=False, sort_keys=True,
                                     separators=(',', ':')).encode('utf-8')).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=pathlib.Path)
    parser.add_argument('--compare', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve(strict=True)
    target = args.output.resolve()
    if target.exists() or target.is_relative_to(root):
        raise ValueError('Output must be new and outside inventoried root (no self-reference)')
    rows = inventory(root)
    report = {'fileCount': len(rows), 'treeSHA256': identity(rows), 'entries': rows}
    if args.compare:
        other = inventory(args.compare)
        report['exactMatch'] = rows == other
        report['otherTreeSHA256'] = identity(other)
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    if report.get('exactMatch') is False:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
