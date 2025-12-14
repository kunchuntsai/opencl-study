#!/usr/bin/env python3
"""
Download D3.js for offline HTML reports.

Run this script once to enable offline viewing of generated HTML reports.
After running, the analyzer will automatically embed D3.js in generated HTML files.

Usage:
    python3 download_d3.py
"""

import os
import sys
import urllib.request

D3_VERSION = "7.8.5"
D3_URL = f"https://d3js.org/d3.v7.min.js"
D3_CDN_URLS = [
    f"https://d3js.org/d3.v7.min.js",
    f"https://cdn.jsdelivr.net/npm/d3@7/dist/d3.min.js",
    f"https://unpkg.com/d3@7/dist/d3.min.js",
    f"https://cdnjs.cloudflare.com/ajax/libs/d3/{D3_VERSION}/d3.min.js",
]

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_FILE = os.path.join(SCRIPT_DIR, "d3.v7.min.js")


def download_d3():
    """Download D3.js from available CDNs."""
    print(f"Downloading D3.js v{D3_VERSION} for offline use...")
    print()

    for url in D3_CDN_URLS:
        print(f"Trying: {url}")
        try:
            urllib.request.urlretrieve(url, OUTPUT_FILE)
            size = os.path.getsize(OUTPUT_FILE)
            if size > 100000:  # D3.js should be > 100KB
                print(f"Success! Downloaded {size:,} bytes")
                print(f"Saved to: {OUTPUT_FILE}")
                print()
                print("Offline mode enabled. Generated HTML reports will now")
                print("include D3.js inline and work without internet.")
                return True
            else:
                os.remove(OUTPUT_FILE)
                print(f"File too small ({size} bytes), trying next...")
        except Exception as e:
            print(f"Failed: {e}")
            if os.path.exists(OUTPUT_FILE):
                os.remove(OUTPUT_FILE)
        print()

    print("Error: Could not download D3.js from any CDN.")
    print()
    print("Manual download instructions:")
    print(f"1. Visit https://d3js.org/d3.v7.min.js in your browser")
    print(f"2. Save the file as: {OUTPUT_FILE}")
    return False


if __name__ == "__main__":
    success = download_d3()
    sys.exit(0 if success else 1)
