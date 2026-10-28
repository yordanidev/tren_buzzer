import os
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

PORT = 5000
SOUNDS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "sounds")

pending_file = None


def list_sounds():
    if not os.path.isdir(SOUNDS_DIR):
        return []
    return sorted(f for f in os.listdir(SOUNDS_DIR) if f.lower().endswith(".mp3"))


def render_html():
    buttons = "\n".join(
        f'<button onclick="playSound(\'{name}\')">{name}</button>'
        for name in list_sounds()
    )
    if not buttons:
        buttons = "<p>No hay sonidos en la carpeta sounds/</p>"
    return f"""<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Biblioteca de sonidos</title>
<style>
body {{ font-family: sans-serif; text-align: center; background: #111; color: #eee; padding-top: 40px; }}
button {{ display: block; width: 85%; margin: 10px auto; font-size: 20px; padding: 20px; border-radius: 12px; border: none; background: #e63946; color: white; }}
button:active {{ background: #b3242f; }}
</style>
</head>
<body>
<h1>Biblioteca de sonidos</h1>
{buttons}
<p id="status"></p>
<script>
function playSound(name) {{
  document.getElementById('status').innerText = 'Enviado...';
  fetch('/play?file=' + encodeURIComponent(name)).then(() => {{
    document.getElementById('status').innerText = 'Listo';
  }});
}}
</script>
</body>
</html>
""".encode()


class Handler(BaseHTTPRequestHandler):
    def _send_text(self, code, text):
        body = text.encode()
        self.send_response(code)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        global pending_file
        parsed = urlparse(self.path)

        if parsed.path == "/":
            body = render_html()
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        elif parsed.path == "/play":
            params = parse_qs(parsed.query)
            name = params.get("file", [None])[0]
            if name in list_sounds():
                pending_file = name
                self._send_text(200, "ok")
            else:
                self._send_text(400, "archivo invalido")

        elif parsed.path == "/should_play":
            if pending_file:
                name = pending_file
                pending_file = None
                self._send_text(200, name)
            else:
                self._send_text(200, "0")

        elif parsed.path == "/stream":
            params = parse_qs(parsed.query)
            name = params.get("file", [None])[0]
            if name not in list_sounds():
                self._send_text(404, "no encontrado")
                return
            path = os.path.join(SOUNDS_DIR, name)
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Connection", "close")
            self.end_headers()
            proc = subprocess.Popen(
                [
                    "gst-launch-1.0", "-q",
                    "filesrc", f"location={path}", "!",
                    "decodebin", "!", "audioconvert", "!", "audioresample", "!",
                    "audio/x-raw,format=S16LE,channels=2,rate=44100", "!",
                    "fdsink", "fd=1", "sync=false",
                ],
                stdout=subprocess.PIPE,
            )
            try:
                while True:
                    chunk = proc.stdout.read(4096)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
            except (BrokenPipeError, ConnectionResetError):
                pass
            finally:
                proc.terminate()
                proc.wait()

        else:
            self._send_text(404, "not found")

    def log_message(self, format, *args):
        pass


if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", PORT), Handler)
    print(f"Servidor corriendo en http://0.0.0.0:{PORT}")
    print(f"Biblioteca de sonidos: {SOUNDS_DIR}")
    server.serve_forever()
