#!/usr/bin/env python3
"""Serve console/ for the prototype, and expose repo Markdown read-only at /__src/.

The prototype needs two things the plain `http.server` cannot give it:

  1. `../data/*.json` and `../content/*.json` inside console/  — a normal static serve covers this
  2. the ORIGINAL Markdown (`wiki/07-…/17-mysql….md`) so "定位原文" can show the real line and
     highlight it — that lives outside console/, one level up

`serve.py` maps `/__src/<repo-relative path>.md` to the repository root, **read-only**, and refuses
anything that is not a `.md` file inside the allowed content roots. It never exposes `.git`,
`.env`, or the vendor checkouts.

Usage:
  python3 console/prototype/serve.py            # http://127.0.0.1:8766/prototype/
  python3 console/prototype/serve.py --port 8770
"""

from __future__ import annotations

import argparse
import http.server
import pathlib
import urllib.parse

CONSOLE = pathlib.Path(__file__).resolve().parent.parent
REPO = CONSOLE.parent
#: Only Markdown under these top-level roots may be served through /__src/.
ALLOWED_ROOTS = ("wiki", "skills", "maintaining", "code")
#: Vendor checkouts are never content (they are rebuilt by `make repo-sync`).
DENIED_PREFIXES = ("code/repo/",)


class Handler(http.server.SimpleHTTPRequestHandler):
    """Static files from console/, plus a read-only /__src/ bridge to the repo's Markdown."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(CONSOLE), **kwargs)

    def do_GET(self) -> None:  # noqa: N802 (http.server API)
        path = urllib.parse.urlsplit(self.path).path
        if path.startswith("/__src/"):
            return self._serve_source(path[len("/__src/"):])
        return super().do_GET()

    def _serve_source(self, rel: str) -> None:
        rel = urllib.parse.unquote(rel)
        target = (REPO / rel).resolve()
        rel_posix = target.relative_to(REPO).as_posix() if target.is_relative_to(REPO) else ""
        allowed = (
            target.is_file()
            and target.suffix == ".md"
            and any(target.is_relative_to((REPO / root).resolve()) for root in ALLOWED_ROOTS)
            and not rel_posix.startswith(DENIED_PREFIXES)
        )
        if not allowed:
            self.send_error(404, "not a servable Markdown file")
            return
        data = target.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", "text/markdown; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, fmt: str, *args) -> None:  # keep the console quiet
        if any(isinstance(a, str) and "/__src/" in a for a in args):
            return
        super().log_message(fmt, *args)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=8766)
    parser.add_argument("--bind", default="127.0.0.1")
    args = parser.parse_args()
    server = http.server.ThreadingHTTPServer((args.bind, args.port), Handler)
    print(f"console prototype → http://{args.bind}:{args.port}/prototype/")
    print(f"source bridge    → /__src/<repo-relative>.md  (read-only, {', '.join(ALLOWED_ROOTS)})")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
