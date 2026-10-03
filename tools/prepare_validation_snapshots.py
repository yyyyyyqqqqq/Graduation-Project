"""Explicit public-data preparation only; no product runtime or private project inputs.

python tools/prepare_validation_snapshots.py <ignored-output-directory>
Downloads each official ecosystem archive once, preserving exact bytes and HTTP metadata.
Existing outputs are never overwritten. C++ ValidationPrepare verifies archive/record hashes.
"""
import concurrent.futures
import datetime
import hashlib
import json
from pathlib import Path
import sys
import urllib.request
import zipfile


def download(ecosystem, directory):
    url = f"https://storage.googleapis.com/osv-vulnerabilities/{ecosystem}/all.zip"
    archive = directory / f"{ecosystem}-all.zip"
    records_path = directory / f"{ecosystem}-records.json"
    source_path = directory / f"{ecosystem}-source.json"
    if any(p.exists() for p in (archive, records_path, source_path)):
        raise FileExistsError(f"Refusing replacement: {ecosystem}")
    digest = hashlib.sha256()
    size = 0
    with urllib.request.urlopen(url, timeout=60) as response, archive.open("xb") as output:
        headers = {key: response.headers.get(key) for key in
                   ("Last-Modified", "ETag", "x-goog-generation", "Content-Length")}
        while chunk := response.read(1024 * 1024):
            output.write(chunk)
            digest.update(chunk)
            size += len(chunk)
    downloaded = datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")
    with zipfile.ZipFile(archive) as z:
        records = [json.loads(z.read(name)) for name in sorted(z.namelist()) if name.endswith(".json")]
    records_bytes = json.dumps(records, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    with records_path.open("xb") as output:
        output.write(records_bytes)
    source = dict(sourceIdentity=url, ecosystem=ecosystem, downloadedAtUtc=downloaded,
                  sourceSHA256=digest.hexdigest(), sourceSizeBytes=size, upstreamMetadata=headers,
                  recordsSHA256=hashlib.sha256(records_bytes).hexdigest())
    with source_path.open("x", encoding="utf-8") as output:
        json.dump(source, output, ensure_ascii=False, indent=2)
    print(f"{ecosystem}: {len(records)} records; SHA-256 {digest.hexdigest()}", flush=True)


def verify_existing(ecosystem, directory):
    """Verify initial downloaded archives/extractions before pool freeze; preserve initial provenance."""
    source_path = directory / f"{ecosystem}-source.json"
    source_bytes = source_path.read_bytes()
    source = json.loads(source_bytes)
    archive = directory / f"{ecosystem}-all.zip"
    with archive.open("rb") as stream:
        archive_hash = hashlib.file_digest(stream, "sha256").hexdigest()
    if archive_hash != source["sourceSHA256"] or archive.stat().st_size != source["sourceSizeBytes"]:
        raise ValueError("Exact archive hash/size mismatch")
    with zipfile.ZipFile(archive) as z:
        records = [json.loads(z.read(name)) for name in sorted(z.namelist()) if name.endswith(".json")]
    actual = (directory / f"{ecosystem}-records.json").read_bytes()
    if json.loads(actual) != records:
        raise ValueError("Extraction differs from exact official archive")
    with (directory / f"{ecosystem}-source-download.json").open("xb") as backup:
        backup.write(source_bytes)
    source["recordsSHA256"] = hashlib.sha256(actual).hexdigest()
    if "metadata" in source:
        source["upstreamMetadata"] = source.pop("metadata")
    source_path.write_text(json.dumps(source, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Verified {ecosystem}: exact archive and {len(records)} extracted records", flush=True)


if __name__ == "__main__":
    directory = Path(sys.argv[1])
    directory.mkdir(parents=True, exist_ok=True)
    operation = verify_existing if "--verify-existing" in sys.argv[2:] else download
    # Verification can hold large decoded archives; do it sequentially.
    if operation == verify_existing:
        for ecosystem in ("PyPI", "npm"):
            operation(ecosystem, directory)
    else:
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            list(pool.map(lambda eco: operation(eco, directory), ("PyPI", "npm")))
