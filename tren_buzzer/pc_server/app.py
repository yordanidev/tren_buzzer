from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

PORT = 5000
SOUNDS = ["tren", "barco"]

pending_sound = None

INDEX_HTML = """<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sonidos ESP32</title>
<style>
body {{ font-family: sans-serif; text-align: center; background: #111; color: #eee; padding-top: 60px; }}
button {{ font-size: 24px; padding: 25px 40px; border-radius: 16px; border: none; background: #e63946; color: white; margin: 10px; }}
button:active {{ background: #b3242f; }}
</style>
</head>
<body>
<h1>Sonidos</h1>
{buttons}
<p id="status"></p>
<script>
function playSound(name) {{
  document.getElementById('status').innerText = 'Enviado...';
  fetch('/play?sound=' + name).then(() => {{
    document.getElementById('status').innerText = 'Listo';
  }});
}}
</script>
</body>
</html>
"""


def render_html():
    buttons = "\n".join(
        f'<button onclick="playSound(\'{name}\')">{name.upper()}</button>'
        for name in SOUNDS
    )
    return INDEX_HTML.format(buttons=buttons).encode()


class Handler(BaseHTTPRequestHandler):
    def _send(self, code, body, ctype="text/plain"):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        global pending_sound
        parsed = urlparse(self.path)
        if parsed.path == "/":
            self._send(200, render_html(), "text/html")
        elif parsed.path == "/play":
            params = parse_qs(parsed.query)
            sound = params.get("sound", [None])[0]
            if sound in SOUNDS:
                pending_sound = sound
                self._send(200, b"ok")
            else:
                self._send(400, b"sonido invalido")
        elif parsed.path == "/should_play":
            if pending_sound:
                body = pending_sound.encode()
                pending_sound = None
                self._send(200, body)
            else:
                self._send(200, b"0")
        else:
            self._send(404, b"not found")

    def log_message(self, format, *args):
        pass


if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", PORT), Handler)
    print(f"Servidor corriendo en http://0.0.0.0:{PORT}")
    server.serve_forever()
