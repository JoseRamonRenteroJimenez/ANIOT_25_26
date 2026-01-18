import http.server
import os, sys
import ssl
from typing import Optional


def start_https_server(
    serve_dir: str,
    server_ip: str,
    server_port: int,
    cert_file: str,
    key_file: str,
    handler: Optional[type] = None,
) -> None:
    """
    Start a simple HTTPS file server.

    :param serve_dir: Directory to serve files from
    :param server_ip: IP address to bind (e.g. '0.0.0.0')
    :param server_port: TCP port
    :param cert_file: Path to PEM certificate
    :param key_file: Path to PEM private key
    :param handler: Optional HTTP request handler
    """

    if not os.path.isdir(serve_dir):
        raise ValueError(f"Invalid serve directory: {serve_dir}")

    if not os.path.isfile(cert_file):
        raise ValueError(f"Certificate file not found: {cert_file}")

    if not os.path.isfile(key_file):
        raise ValueError(f"Key file not found: {key_file}")

    os.chdir(serve_dir)

    if handler is None:
        handler = http.server.SimpleHTTPRequestHandler

    httpd = http.server.HTTPServer((server_ip, server_port), handler)

    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ssl_context.load_cert_chain(
        certfile=cert_file,
        keyfile=key_file,
    )

    httpd.socket = ssl_context.wrap_socket(
        httpd.socket,
        server_side=True,
    )

    print(f"HTTPS server running at https://{server_ip}:{server_port}")
    print(f"Serving directory: {serve_dir}")

    httpd.serve_forever()


if __name__ == "__main__":

    this_dir = os.path.dirname(os.path.realpath(__file__))

    if len(sys.argv) == 1:
        serve_dir = os.path.join(this_dir, "bin")
        port = 8000
        cert_dir = os.path.join(this_dir, "certs")
        print("No arguments provided, using default configuration")
    else:
        if len(sys.argv) < 3:
            print("Usage: python https_server.py <image_dir> <port> [cert_dir]")
            sys.exit(1)

        serve_dir = os.path.join(this_dir, sys.argv[1])
        port = int(sys.argv[2])
        cert_dir = serve_dir if len(sys.argv) < 4 else os.path.join(this_dir, sys.argv[3])

    cert_file = os.path.join(cert_dir, "ca_cert.pem")
    key_file = os.path.join(cert_dir, "ca_key.pem")


    start_https_server(
        serve_dir=serve_dir,
        server_ip="0.0.0.0",
        server_port=port,
        cert_file=cert_file,
        key_file=key_file,
    )
