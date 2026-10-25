"""Dump the Figma design into design/ so it can be read offline.

Usage: python tools/figma_export.py [file|index|frames|icons|fills|all]

Each step skips work whose output already exists, so re-running after a rate
limit only spends requests on what is missing. Delete design/ to refresh.
Reads FIGMA_TOKEN from the environment (or the Windows user environment).
"""
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

FILE_KEY = "q8hLRaU8oyhvehEUA2OR7f"
API = "https://api.figma.com/v1"
OUT = Path(__file__).resolve().parent.parent / "design"
FRAME_TYPES = {"FRAME", "COMPONENT", "COMPONENT_SET", "INSTANCE", "GROUP"}


def token():
    value = os.environ.get("FIGMA_TOKEN")
    if not value and sys.platform == "win32":
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as key:
            value = winreg.QueryValueEx(key, "FIGMA_TOKEN")[0]
    if not value:
        sys.exit("FIGMA_TOKEN is not set")
    return value


def api_get(path, **params):
    url = f"{API}{path}"
    if params:
        url += "?" + urllib.parse.urlencode(params)
    request = urllib.request.Request(url, headers={"X-Figma-Token": token()})
    try:
        with urllib.request.urlopen(request, timeout=300) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        body = error.read().decode(errors="replace")[:500]
        retry = error.headers.get("Retry-After")
        extra = {k: v for k, v in error.headers.items() if k.lower().startswith("x-figma")}
        sys.exit(f"HTTP {error.code} for {path}: {body}\nRetry-After: {retry} {extra}")


def download(url, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    for attempt in range(3):
        try:
            with urllib.request.urlopen(url, timeout=300) as response:
                target.write_bytes(response.read())
            return
        except urllib.error.URLError:
            time.sleep(2 * (attempt + 1))
    print(f"  failed to download {target.name}")


def slug(text):
    text = re.sub(r"[^\w\-=,. ]+", "_", text).strip()
    return re.sub(r"\s+", " ", text)[:80] or "unnamed"


def file_name(node):
    return f"{slug(node['name'])} [{node['id'].replace(':', '-')}]"


def load_file():
    path = OUT / "figma.json"
    if not path.exists():
        print("Fetching the full document tree...")
        data = api_get(f"/files/{FILE_KEY}", geometry="paths")
        OUT.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(data, indent=1, ensure_ascii=False), encoding="utf-8")
        print(f"  saved {path.stat().st_size / 1e6:.1f} MB")
    return json.loads(path.read_text(encoding="utf-8"))


def top_frames(document):
    """Top-level frames of every page, looking inside sections."""
    def walk(nodes, page):
        for node in nodes:
            if node["type"] == "SECTION":
                yield from walk(node.get("children", []), page)
            elif node["type"] in FRAME_TYPES:
                yield page, node
    for page in document["children"]:
        yield from walk(page.get("children", []), page)


def components(document):
    """Every component, with its component set when it belongs to one."""
    def walk(node, parent_set):
        if node["type"] == "COMPONENT":
            yield parent_set, node
        current = node if node["type"] == "COMPONENT_SET" else parent_set
        for child in node.get("children", []):
            yield from walk(child, current)
    yield from walk(document, None)


def render(nodes, fmt, scale, batch):
    """Render (node, target) pairs, skipping ones already on disk."""
    pending = [(node, target) for node, target in nodes if not target.exists()]
    print(f"  {len(nodes) - len(pending)} cached, {len(pending)} to render")
    for start in range(0, len(pending), batch):
        chunk = pending[start:start + batch]
        ids = ",".join(node["id"] for node, _ in chunk)
        result = api_get(f"/images/{FILE_KEY}", ids=ids, format=fmt, scale=scale,
                         svg_outline_text="false", svg_include_id="true")
        if result.get("err"):
            print(f"  render error: {result['err']}")
        urls = result.get("images", {})
        for node, target in chunk:
            if urls.get(node["id"]):
                download(urls[node["id"]], target)
            else:
                print(f"  no render for {node['name']} ({node['id']})")


def step_index(data):
    lines = [f"# {data['name']}", "", f"Last modified: {data.get('lastModified')}", ""]
    for page in data["document"]["children"]:
        lines += [f"## Page: {page['name']} ({page['id']})", ""]
        for p, frame in top_frames(data["document"]):
            if p is page:
                box = frame.get("absoluteBoundingBox") or {}
                lines.append(f"- `{frame['id']}` {frame['type']} **{frame['name']}** "
                             f"{box.get('width', 0):.0f}x{box.get('height', 0):.0f}")
        lines.append("")
    lines += ["## Components", ""]
    for parent, comp in components(data["document"]):
        prefix = f"{parent['name']} / " if parent else ""
        lines.append(f"- `{comp['id']}` {prefix}{comp['name']}")
    lines += ["", "## Styles", ""]
    for style_id, style in data.get("styles", {}).items():
        lines.append(f"- `{style_id}` {style['styleType']} {style['name']}")
    (OUT / "index.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("Wrote index.md")


def step_frames(data):
    print("Rendering top-level frames as PNG @2x...")
    nodes = [(frame, OUT / "frames" / slug(page["name"]) / f"{file_name(frame)}.png")
             for page, frame in top_frames(data["document"])]
    render(nodes, "png", 2, batch=40)


def step_icons(data):
    print("Rendering components as SVG...")
    nodes = []
    for parent, comp in components(data["document"]):
        folder = OUT / "components" / (file_name(parent) if parent else "_standalone")
        nodes.append((comp, folder / f"{file_name(comp)}.svg"))
    render(nodes, "svg", 1, batch=150)


def step_fills(data):
    print("Downloading image fills...")
    images = api_get(f"/files/{FILE_KEY}/images").get("meta", {}).get("images", {})
    for ref, url in images.items():
        target = OUT / "fills" / f"{ref}.png"
        if not target.exists() and url:
            download(url, target)
    print(f"  {len(images)} image fills")


def main():
    step = sys.argv[1] if len(sys.argv) > 1 else "all"
    data = load_file()
    if step in ("index", "all"):
        step_index(data)
    if step in ("frames", "all"):
        step_frames(data)
    if step in ("icons", "all"):
        step_icons(data)
    if step in ("fills", "all"):
        step_fills(data)


if __name__ == "__main__":
    main()
