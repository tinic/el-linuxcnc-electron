#!/usr/bin/env python3
"""Regenerate the committed API schema from api_models.py.

Usage: python3 generate_schema.py
Writes ../api-schema.json (consumed by `yarn generate:api` in
elle-frontend to produce src/api/types.gen.ts).
"""

import json
from pathlib import Path

from api_models import json_schema


def main() -> None:
    out = Path(__file__).resolve().parent.parent / "api-schema.json"
    out.write_text(json.dumps(json_schema(), indent=2, sort_keys=True) + "\n")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
